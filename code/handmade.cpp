#include "handmade.h"

internal int32 RoundReal32toInt32(real32 Real32)
{
    int32 Result = (int32)(Real32 + 0.5f);
    return Result;
}

internal void DrawRectangle(game_offscreen_buffer *Buffer,
                            real32 RealMinX, real32 RealMinY,
                            real32 RealMaxX, real32 RealMaxY,
                            real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32toInt32(RealMinX);
    int32 MaxX = RoundReal32toInt32(RealMaxX);
    int32 MinY = RoundReal32toInt32(RealMinY);
    int32 MaxY = RoundReal32toInt32(RealMaxY);
    
    
    if (MinX < 0)
    {
        MinX = 0;
    }

    if (MinY < 0)
    {
        MinY = 0;
    }

    // NOTE: if out of frame there are could be 2 cases:
    // 1) daraw rect on the edge
    // 2) do NOT draw
    // here is implemented 2nd case

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8 *Row = ((uint8 *)Buffer->Memory + 
                    MinX * Buffer->BytesPerPixel + 
                    MinY * Buffer->Pitch);
    uint32 Color = (uint32)(
        RoundReal32toInt32(R * 255.0f) << 16 |
        RoundReal32toInt32(G * 255.0f) << 8 |
        RoundReal32toInt32(B * 255.0f)
    );
    for (int32 y = MinY; y < MaxY; y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int32 x = MinX; x < MaxX; x++)
        {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}

bool CanMove(int32 TileXCount, int32 TileYCount, 
            int32 TileWidth, int32 TileHeight, 
            uint32 *Tiles, 
            real32 PointX, real32 PointY)
{
    int32 Col = (int32)(PointX / TileWidth);
    int32 Row = (int32)(PointY / TileHeight);

    uint32 Tile = Tiles[Row*TileXCount + Col];
    return !Tile;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {
        Memory->isInitialized = true;
        GameState->PlayerX = 150;
        GameState->PlayerY = 50;
    }
    
    DrawRectangle(Buffer, 
            0.0f, 0.0f, 
            (real32)Buffer->Width, (real32)Buffer->Height,
            1.0f, 0.0f, 1.0f);

    uint32 TileMap[9][16] = 
    {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,1,0,0, 0,0,1,1, 0,0,1,0, 0,0,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
    };
    
    int TileWidth = Buffer->Width / 16;
    int TileHeight = Buffer->Height / 9;
    for (int Row = 0; Row < 9; Row++)
    {
        for (int Col = 0; Col < 16; Col++)
        {
            
            DrawRectangle(Buffer, 
                (real32)Col * TileWidth, (real32)Row * TileHeight, 
                (real32)(Col +1) * TileWidth , (real32)(Row+1) * TileHeight,
                0.0f, (real32)TileMap[Row][Col], 1.0f);
        }
    }

    game_controller_input Input0 = Input->Controllers[0];

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerWidth = 0.5f * (real32)TileWidth;
    real32 PlayerHeight = (real32)TileHeight;
    real32 PlayerHeadOffset = 0.3f * (real32)TileHeight;

    real32 dPlayerX = 0.0f;
    real32 dPlayerY = 0.0f;
    real32 Delta = 5.0f;

    real32 NewPlayerX = GameState->PlayerX;
    real32 NewPlayerY = GameState->PlayerY;
    if (Input0.Up.EndedDown)
    {
        dPlayerY = -Delta;
    }
    if (Input0.Down.EndedDown)
    {
        dPlayerY = Delta;
    }
    if (Input0.Left.EndedDown)
    {
        dPlayerX = -Delta;
    }
    if (Input0.Right.EndedDown)
    {
        dPlayerX = Delta;
    }

    NewPlayerX += dPlayerX;
    NewPlayerY += dPlayerY;

    if (
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, NewPlayerX, GameState->PlayerY+PlayerHeadOffset) &&
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, NewPlayerX+PlayerWidth, GameState->PlayerY+PlayerHeadOffset) &&
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, NewPlayerX, GameState->PlayerY+PlayerHeight) &&
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, NewPlayerX+PlayerWidth, GameState->PlayerY+PlayerHeight)
        )
    {
        GameState->PlayerX = NewPlayerX;
    }

    if (
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, GameState->PlayerX, NewPlayerY+PlayerHeadOffset) &&
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, GameState->PlayerX+PlayerWidth, NewPlayerY+PlayerHeadOffset) &&
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, GameState->PlayerX, NewPlayerY+PlayerHeight) &&
        CanMove(16, 9, TileWidth, TileHeight, (uint32 *)TileMap, GameState->PlayerX+PlayerWidth, NewPlayerY+PlayerHeight)
        )
    {
        GameState->PlayerY = NewPlayerY;
    }
    
    
    DrawRectangle(Buffer, GameState->PlayerX, GameState->PlayerY, 
                GameState->PlayerX + PlayerWidth, GameState->PlayerY + PlayerHeight,
                PlayerR, PlayerG, PlayerB);

    DrawRectangle(Buffer, 
                (real32)Input->MouseX, (real32)Input->MouseY, 
                (real32)Input->MouseX + 10, (real32)Input->MouseY + 10,
                1.0f, 1.0f, 1.0f);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    
}

/*
internal void OutputSound(
    game_sound_output_buffer *SoundBuffer,
    game_state *GameState,
    int ToneVolume)
{
    int WavePeriod = SoundBuffer->SamplesPerSecond/GameState->ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 sineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(sineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f * win32_PI * 1.0f / (real32)WavePeriod;
        if (GameState->tSine > 2.0f*win32_PI)
        {
            GameState->tSine -= 2.0f*win32_PI;
        }
    }
}

internal void RenderGradient(
    game_offscreen_buffer *Buffer, 
    int xOffset, 
    int yOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int y = 0; y < Buffer->Height; y++)
    {
        uint32 *Pixel = (uint32 *)Row;

        for (int x = 0; x < Buffer->Width; x++)
        {
            uint8 Red = (uint8)(x + xOffset);
            uint8 Green = (uint8)(y + xOffset);
            uint8 Blue = (uint8)(y + yOffset);
            *Pixel = ((Red << 16) | (Green << 8) | Blue);
            ++Pixel;
        }
        Row += Buffer->Pitch;
    }
}
*/