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

inline void RecanonicalazeCoord(int32 MapDim, int32 TileDim, int32 *TileMap, int32 *Tile, real32 *TileRel)
{
    int32 Offset = (int32)floorf(*TileRel / (real32)TileDim);
    *Tile += Offset;
    *TileRel -= TileDim * Offset;
    if (*Tile < 0)
    {
        *Tile = MapDim + *Tile;
        --*TileMap;
    }

    if (*Tile >= MapDim)
    {
        *Tile = MapDim - *Tile;
        ++*TileMap;
    }
}

inline void RecanonicalizePostion(world *World, canonical_postition *Pos)
{
    RecanonicalazeCoord(World->TileMapCountX, World->TileSideInPixels, &Pos->TileMapX, &Pos->TileX, &Pos->RelTileX);
    RecanonicalazeCoord(World->TileMapCountY, World->TileSideInPixels, &Pos->TileMapY, &Pos->TileY, &Pos->RelTileY);
}

inline tile_map *GetTileMap(world *World, int32 X, int32 Y)
{
    tile_map *TileMap = &World->TileMaps[Y * World->CountX + X];
    return TileMap;
}

bool CanMove(world *World, canonical_postition *Pos)
{
    tile_map *TileMap = GetTileMap(World, Pos->TileMapX, Pos->TileMapY); 
    uint32 Tile = TileMap->Map[Pos->TileY *World->TileMapCountX + Pos->TileX];
    return !Tile;
}

#define TILE_MAP_COUNT_X 16
#define TILE_MAP_COUNT_Y 9
#define TILES_COUNT_X 2
#define TILES_COUNT_Y 2

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {
        Memory->isInitialized = true;
        GameState->PlayerP.RelTileX = 3;
        GameState->PlayerP.RelTileY = 3;
        GameState->PlayerP.TileX = 3;
        GameState->PlayerP.TileY = 3;
        GameState->PlayerP.TileMapX = 0;
        GameState->PlayerP.TileMapY = 0;
    }
    
    DrawRectangle(Buffer, 
            0.0f, 0.0f, 
            (real32)Buffer->Width, (real32)Buffer->Height,
            1.0f, 0.0f, 1.0f);

    world World = {};
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.PixPerMeter = (int32)((real32)World.TileSideInPixels / World.TileSideInMeters);
    World.TileMapCountX = TILE_MAP_COUNT_X;
    World.TileMapCountY = TILE_MAP_COUNT_Y;

    uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = 
    {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,1,0,0, 0,0,1,1, 0,0,1,0, 0,0,0,0},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
    };

    uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = 
    {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {0,0,0,0, 0,0,1,1, 0,0,1,0, 0,0,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
    };

    uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = 
    {
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,1,0,0, 0,0,1,1, 0,0,1,0, 0,0,0,0},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
    };

    uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = 
    {
        {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,1, 1,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},

        {1,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,1},
        {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,1},
        {1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},

        {1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
        {1,0,0,0, 0,0,0,0, 0,1,1,1, 1,1,1,1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
    };
    tile_map Tiles[TILES_COUNT_Y][TILES_COUNT_X];
    Tiles[0][0].Map = (uint32 *)Tiles00;
    Tiles[0][1].Map = (uint32 *)Tiles01;
    Tiles[1][0].Map = (uint32 *)Tiles10;
    Tiles[1][1].Map = (uint32 *)Tiles11;

    World.TileMaps = (tile_map *)Tiles;
    World.CountX = TILES_COUNT_X;
    World.CountY = TILES_COUNT_Y;
    

    game_controller_input Input0 = Input->Controllers[0];
    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerWidth = 0.5f * (real32)World.TileSideInPixels;
    real32 PlayerHeight = (real32)World.TileSideInPixels;
    real32 PlayerHeadOffset = 0.3f * (real32)World.TileSideInPixels;

    real32 dPlayerX = 0.0f;
    real32 dPlayerY = 0.0f;
    real32 DeltaMPerSec = 5.0f;
    real32 Delta = (real32)(DeltaMPerSec * Input->SecondsToAdvance * World.PixPerMeter);

    real32 NewPlayerX = GameState->PlayerP.RelTileX;
    real32 NewPlayerY = GameState->PlayerP.RelTileY;
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

    if (NewPlayerX != GameState->PlayerP.RelTileX || NewPlayerY != GameState->PlayerP.RelTileY)
    {
        canonical_postition BottomLeft = GameState->PlayerP;
        BottomLeft.RelTileX = NewPlayerX;
        BottomLeft.RelTileY = NewPlayerY + PlayerHeight;
        canonical_postition BottomRight = BottomLeft;
        BottomRight.RelTileX += PlayerWidth;
        RecanonicalizePostion(&World, &BottomLeft);
        RecanonicalizePostion(&World, &BottomRight);
        if (
            CanMove(&World, &BottomLeft) &&
            CanMove(&World, &BottomRight))
        {
            canonical_postition TopRight = GameState->PlayerP;
            TopRight.RelTileX = NewPlayerX;
            TopRight.RelTileY = NewPlayerY;
            RecanonicalizePostion(&World, &TopRight);

            GameState->PlayerP.TileX    = TopRight.TileX;
            GameState->PlayerP.TileY    = TopRight.TileY;
            GameState->PlayerP.TileMapX = TopRight.TileMapX;
            GameState->PlayerP.TileMapY = TopRight.TileMapY;
            GameState->PlayerP.RelTileX = TopRight.RelTileX;
            GameState->PlayerP.RelTileY = TopRight.RelTileY;
        }
    }
    
    tile_map *TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);
    for (int Row = 0; Row < World.TileMapCountY; Row++)
    {
        for (int Col = 0; Col < World.TileMapCountX; Col++)
        {
            real32 Red = 0.0f;
            if (Row == GameState->PlayerP.TileY && Col == GameState->PlayerP.TileX)
            {
                Red = 1.0f;
            }
            DrawRectangle(Buffer, 
                (real32)Col * World.TileSideInPixels, (real32)Row * World.TileSideInPixels, 
                (real32)(Col + 1) * World.TileSideInPixels , (real32)(Row + 1) * World.TileSideInPixels,
                Red, (real32)TileMap->Map[Row * World.TileMapCountX + Col], 1.0f);
        }
    }

    real32 WorldTopLeftX = GameState->PlayerP.TileX * World.TileSideInPixels + GameState->PlayerP.RelTileX;
    real32 WorldTopLeftY = GameState->PlayerP.TileY * World.TileSideInPixels + GameState->PlayerP.RelTileY;
    DrawRectangle(Buffer, WorldTopLeftX, WorldTopLeftY, 
                WorldTopLeftX + PlayerWidth, WorldTopLeftY + PlayerHeight,
                PlayerR, PlayerG, PlayerB);
    DrawPoint(Buffer, WorldTopLeftX, WorldTopLeftY, 1.0f, 0.0f, 0.0f);

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