#include "handmade.h"
#include "corecrt_math.h"
#include "tile.cpp"

internal void DrawPoint(game_offscreen_buffer *Buffer,
                        real32 RealX, real32 RealY,
                        real32 R, real32 G, real32 B)
{
    int32 X = RoundReal32toInt32(RealX);
    int32 Y = RoundReal32toInt32(RealY);

    if (X < 0)
    {
        X = 0;
    }

    if (Y < 0)
    {
        Y = 0;
    }
    if (X > Buffer->Width)
    {
        X = Buffer->Width;
    }

    if (Y > Buffer->Height)
    {
        Y = Buffer->Height;
    }

    uint32 Color = (uint32)(
        RoundReal32toInt32(R * 255.0f) << 16 |
        RoundReal32toInt32(G * 255.0f) << 8 |
        RoundReal32toInt32(B * 255.0f)
    );
    uint8 *Row = ((uint8 *)Buffer->Memory + 
                    X * Buffer->BytesPerPixel + 
                    Y * Buffer->Pitch);

    uint32 *Pixel = (uint32 *)Row;
    *Pixel++ = Color;
    *Pixel++ = Color;
    *Pixel++ = Color;
    *Pixel = Color;
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

internal void InitializeArena(memory_arena *Arena,
                memory_index Size, uint8 *Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushStruct_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushStruct_(Arena, (Count) * sizeof(type))

void *PushStruct_(memory_arena *Arena, memory_index Size)
{
    Assert(Arena->Used + Size <= Arena->Size);
    void * Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.5f * PlayerHeight;
        
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {
        Memory->isInitialized = true;
        GameState->PlayerP.RelTileX = 5.0f;
        GameState->PlayerP.RelTileY = 5.0f;
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);;


        tile_map *TileMap = World->TileMap;
        
        TileMap->ChunkShift = 8;
        TileMap->ChunkMask = 0xFF;
        TileMap->TileSideInMeters = 1.4f;
        TileMap->TileSideInPixels = 60;
        TileMap->PixPerMeter = RoundReal32toInt32((real32)TileMap->TileSideInPixels / TileMap->TileSideInMeters);
        TileMap->ChunkDim = 256;
        TileMap->TileChunkCountX = 4;
        TileMap->TileChunkCountY = 4;
        TileMap->TileChunks = 
            PushArray(&GameState->WorldArena, TileMap->TileChunkCountX*TileMap->TileChunkCountY, tile_chunk);
        
        for (uint32 Y = 0 ; Y < TileMap->TileChunkCountY; Y++)
        {
            for (uint32 X = 0 ; X < TileMap->TileChunkCountX; X++)
            {
                TileMap->TileChunks[Y*TileMap->TileChunkCountX + X].Map = 
                        PushArray(&GameState->WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
            }
        }

        real32 LowerLeftX = -(real32)TileMap->TileSideInPixels/2;
        real32 LowerLeftY = (real32)Buffer->Height;
        uint32 TilePerWidth = 17;
        uint32 TilePerHeight = 9;
        for (uint32 ScreenY = 0; ScreenY < 32; ScreenY++)
        {
            for (uint32 ScreenX = 0; ScreenX < 32; ScreenX++)
            {
                for (uint32 TileY = 0; TileY < TilePerHeight; TileY++)
                {
                    for (uint32 TileX = 0; TileX < TilePerWidth; TileX++)
                    {
                        uint32 AbsTileX = ScreenX * TilePerWidth + TileX;
                        uint32 AbsTileY = ScreenX * TilePerHeight + TileY;
                        SetTileValue(&GameState->WorldArena, 
                                    World->TileMap, 
                                    AbsTileX, AbsTileY, (TileX + TileY) % 4 ? 0 : 1);
                    }
                }
            }
        }
    }

    game_controller_input Input0 = Input->Controllers[0];
    real32 dPlayerX = 0.0f;
    real32 dPlayerY = 0.0f;
    real32 DeltaMPerSec = 3.0f;
    real32 Delta = (real32)(DeltaMPerSec * Input->SecondsToAdvance);

    real32 NewPlayerX = GameState->PlayerP.RelTileX;
    real32 NewPlayerY = GameState->PlayerP.RelTileY;
    
    
    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;    
    if (Input0.Up.EndedDown)
    {
        dPlayerY = Delta;
    }
    if (Input0.Down.EndedDown)
    {
        dPlayerY = -Delta;
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

    if (NewPlayerX != GameState->PlayerP.RelTileX || NewPlayerY != GameState->PlayerP.RelTileY)
    {
        tile_map_postition BottomLeft = GameState->PlayerP;
        BottomLeft.RelTileX = NewPlayerX - 0.5f*PlayerWidth;
        BottomLeft.RelTileY = NewPlayerY;
        tile_map_postition BottomRight = BottomLeft;
        BottomRight.RelTileX += PlayerWidth;
        RecanonicalizePostion(TileMap, &BottomLeft);
        RecanonicalizePostion(TileMap, &BottomRight);
        if (
            CanMove(TileMap, &BottomLeft) &&
            CanMove(TileMap, &BottomRight))
        {
            tile_map_postition TopRight = GameState->PlayerP;
            TopRight.RelTileX = NewPlayerX;
            TopRight.RelTileY = NewPlayerY;
            RecanonicalizePostion(TileMap, &TopRight);

            GameState->PlayerP.AbsTileX = TopRight.AbsTileX;
            GameState->PlayerP.AbsTileY = TopRight.AbsTileY;

            GameState->PlayerP.RelTileX = TopRight.RelTileX;
            GameState->PlayerP.RelTileY = TopRight.RelTileY;
        }
    }
    
    DrawRectangle(Buffer, 
        0.0f, 0.0f, 
        (real32)Buffer->Width, (real32)Buffer->Height,
        1.0f, 0.0f, 1.0f);
        
    real32 CenterX = (real32)Buffer->Width / 2;
    real32 CenterY = (real32)Buffer->Height / 2;
    for (int32 RelRow = -10; RelRow <  10; RelRow++)
    {
        for (int32 RelCol = - 20; RelCol < 20; RelCol++)
        {
            uint32 Col = RelCol + GameState->PlayerP.AbsTileX;
            uint32 Row = RelRow + GameState->PlayerP.AbsTileY;
            real32 Red = 0.0f;
            if (Row == GameState->PlayerP.AbsTileY && Col == GameState->PlayerP.AbsTileX)
            {
                Red = 1.0f;
            }

            real32 tileCenterX = CenterX - TileMap->PixPerMeter * GameState->PlayerP.RelTileX + ((real32)RelCol)*TileMap->TileSideInPixels;
            real32 tileCenterY = CenterY + TileMap->PixPerMeter * GameState->PlayerP.RelTileY - ((real32)RelRow)*TileMap->TileSideInPixels;
            real32 MinX = tileCenterX - 0.5f * TileMap->TileSideInPixels;
            real32 MinY = tileCenterY - 0.5f * TileMap->TileSideInPixels;
            real32 MaxX = tileCenterX + 0.5f * TileMap->TileSideInPixels;
            real32 MaxY = tileCenterY + 0.5f * TileMap->TileSideInPixels;
            uint32 Blue = GetTileValue(TileMap, Col, Row);
            DrawRectangle(Buffer, 
                MinX, MinY, MaxX, MaxY,
                Red, (real32)Blue, 1.0f);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerLeft = CenterX - PlayerWidth*TileMap->PixPerMeter*0.5f;
    real32 PlayerTop  = CenterY - PlayerHeight*TileMap->PixPerMeter;

    DrawRectangle(Buffer, PlayerLeft, PlayerTop, 
                PlayerLeft + PlayerWidth*TileMap->PixPerMeter, 
                PlayerTop + PlayerHeight*TileMap->PixPerMeter, 
                PlayerR, PlayerG, PlayerB);
   // DrawPoint(Buffer, PlayerLeft + 1, PlayerTop + 1, 1.0, 0.0, 0.0);
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