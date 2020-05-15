#include "handmade.h"
#include "corecrt_math.h"

internal int32 RoundReal32toInt32(real32 Real32)
{
    int32 Result = (int32)(Real32 + 0.5f);
    return Result;
}

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

inline void RecanonicalazeCoord(world *World, uint32 *Tile, real32 *TileRel)
{
    int32 Offset = (int32)floorf(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= World->TileSideInMeters * Offset;
    Assert(*Tile >= 0);
    Assert(*TileRel >= 0);
}

inline void RecanonicalizePostion(world *World, world_postition *Pos)
{
    RecanonicalazeCoord(World, 
        &Pos->AbsTileX, &Pos->RelTileX);
    RecanonicalazeCoord(World, 
        &Pos->AbsTileY, &Pos->RelTileY);
}

inline tile_chunk *GetTileChunk(world *World, int32 X, int32 Y)
{
    tile_chunk* TileChunk = 0;
    if (X >= 0 && X < World->TileChunkCountX && Y >= 0 && Y < World->TileChunkCountY)
    {
        TileChunk = &World->TileChunks[Y * World->TileChunkCountX + X];
    }
    
    return TileChunk;
}

inline tile_chunk_position GetChunkPosition(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> World->ChunkShift;
    Result.TileChunkY = AbsTileY >> World->ChunkShift;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;

    return Result;
}

inline uint32 GetTileValueUnchecked(world *World, tile_chunk *TileChunk, uint32 X, uint32 Y)
{
    Assert(TileChunk)
    Assert(X < World->ChunkDim);
    Assert(Y < World->ChunkDim);
    
    uint32 Value = TileChunk->Map[Y*World->ChunkDim + X];
    return Value;
}

inline uint32 GetTileValue(world *World, tile_chunk *TileChunk, uint32 X, uint32 Y)
{
    
    uint32 Tile = 0;
    if (TileChunk)
    {
        Tile = GetTileValueUnchecked(World, TileChunk, X, Y);
    }
    
    return Tile;
}

internal uint32 GetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position ChunkPos = GetChunkPosition(World, AbsTileX, AbsTileY);
    tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY); 
    uint32 Value = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
    return Value;
}

bool CanMove(world *World, world_postition *Pos)
{
    uint32 Tile = GetTileValue(World, Pos->AbsTileX, Pos->AbsTileY);
    return !Tile;
}

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    

    uint32 Tiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = 
    {
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1, 1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1, 1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1, 1,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,1},
        {1,0,0,0, 0,0,1,1, 0,0,1,0, 0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1, 1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1, 1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1, 1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1, 1,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,1},
        {1,0,0,0, 0,0,1,1, 0,0,1,0, 0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1, 1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
    };

    tile_chunk TileChunk;
    TileChunk.Map = (uint32 *)Tiles;

    world World = {};
    
    World.ChunkShift = 8;
    World.ChunkMask = 0xFF;
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.PixPerMeter = RoundReal32toInt32((real32)World.TileSideInPixels / World.TileSideInMeters);
    World.ChunkDim = 256;
    World.TileChunks = &TileChunk;    
    World.TileChunkCountX = 1;
    World.TileChunkCountY = 1;

    real32 LowerLeftX = -(real32)World.TileSideInPixels/2;
    real32 LowerLeftY = (real32)Buffer->Height;


    game_controller_input Input0 = Input->Controllers[0];
    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerWidth = 0.5f * (real32)World.TileSideInMeters;
    real32 PlayerHeight = (real32)World.TileSideInMeters;
    real32 PlayerHeadOffset = 0.3f * (real32)World.TileSideInMeters;

    real32 dPlayerX = 0.0f;
    real32 dPlayerY = 0.0f;
    real32 DeltaMPerSec = 3.0f;
    real32 Delta = (real32)(DeltaMPerSec * Input->SecondsToAdvance);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {
        Memory->isInitialized = true;
        GameState->PlayerP.RelTileX = 5.0f;
        GameState->PlayerP.RelTileY = 5.0f;
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;
    }

    real32 NewPlayerX = GameState->PlayerP.RelTileX;
    real32 NewPlayerY = GameState->PlayerP.RelTileY;
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
        world_postition BottomLeft = GameState->PlayerP;
        BottomLeft.RelTileX = NewPlayerX - 0.5f*PlayerWidth;
        BottomLeft.RelTileY = NewPlayerY;
        world_postition BottomRight = BottomLeft;
        BottomRight.RelTileX += PlayerWidth;
        RecanonicalizePostion(&World, &BottomLeft);
        RecanonicalizePostion(&World, &BottomRight);
        if (
            CanMove(&World, &BottomLeft) &&
            CanMove(&World, &BottomRight))
        {
            world_postition TopRight = GameState->PlayerP;
            TopRight.RelTileX = NewPlayerX;
            TopRight.RelTileY = NewPlayerY;
            RecanonicalizePostion(&World, &TopRight);

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

            real32 MinX = CenterX + ((real32)RelCol)*World.TileSideInPixels;
            real32 MinY = CenterY - ((real32)RelRow)*World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY - World.TileSideInPixels;
            uint32 Blue = GetTileValue(&World, Col, Row);
            DrawRectangle(Buffer, 
                MinX, MaxY, MaxX, MinY,
                Red, (real32)Blue, 1.0f);
        }
    }

    real32 PlayerLeft = CenterX + World.PixPerMeter * GameState->PlayerP.RelTileX - PlayerWidth*World.PixPerMeter*0.5f;
    real32 PlayerTop  = CenterY - World.PixPerMeter * GameState->PlayerP.RelTileY - PlayerHeight*World.PixPerMeter;

    DrawRectangle(Buffer, PlayerLeft, PlayerTop, 
                PlayerLeft + PlayerWidth*World.PixPerMeter, 
                PlayerTop + PlayerHeight*World.PixPerMeter, 
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