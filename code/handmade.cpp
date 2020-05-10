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

inline tile_map *GetTileMap(world *World, int32 X, int32 Y)
{
    tile_map *TileMap = &World->TileMaps[Y * World->CountX + X];
    return TileMap;
}

bool CanMove(world *World, 
            real32 X, real32 Y, 
            int32 WorldTileX, int32 WorldTileY)
{
    bool Result = true;
    if (Result && X <= 0)
    {    
        if (WorldTileX > 0)
        {
            WorldTileX--;
            X = World->TileWidth*World->TileMapCountX + X;
        }
        else
        {
            Result = false;
        }
    }

    if (Result && X >= World->TileWidth*World->TileMapCountX)
    {    
        if (WorldTileX < World->CountX - 1)
        {
            WorldTileX++;
            X = X - World->TileWidth*World->TileMapCountX;
        }
        else
        {
            Result = false;
        }
    }

    if (Result && Y <= 0)
    {    
        if (WorldTileY > 0)
        {
            WorldTileY--;
            Y = World->TileHeight * World->TileMapCountY + Y;
        }
        else
        {
            Result = false;
        }
    }

    if (Result && Y >= World->TileHeight * World->TileMapCountY)
    {    
        if (WorldTileY < World->CountY - 1)
        {
            WorldTileY++;
            Y = Y - World->TileHeight * World->TileMapCountY;
        }
        else
        {
            Result = false;
        }
    }

    if (Result)
    {
        tile_map *TileMap = GetTileMap(World, WorldTileX, WorldTileY);
        int32 Col = (int32)(X / World->TileWidth);
        int32 Row = (int32)(Y / World->TileHeight);
 
        uint32 Tile = TileMap->Map[Row*World->TileMapCountX + Col];
        return !Tile;
    }
    return Result;
    
}

bool Advance(world *World, world_coordibate *Coord)
{
    bool Result = true;
   if (Result && Coord->X <= 0)
    {    
        if (Coord->WorldTileX > 0)
        {
            Coord->WorldTileX--;
            Coord->X = World->TileWidth*World->TileMapCountX + Coord->X;
        }
        else
        {
            Result = false;
        }
    }

    if (Result && Coord->X >= World->TileWidth*World->TileMapCountX)
    {    
        if (Coord->WorldTileX < World->CountX - 1)
        {
            Coord->WorldTileX++;
            Coord->X = Coord->X - World->TileWidth*World->TileMapCountX;
        }
        else
        {
            Result = false;
        }
    }

    if (Result && Coord->Y <= 0)
    {    
        if (Coord->WorldTileY > 0)
        {
            Coord->WorldTileY--;
            Coord->Y = World->TileHeight * World->TileMapCountY + Coord->Y;
        }
        else
        {
            Result = false;
        }
    }

    if (Result && Coord->Y >= World->TileHeight * World->TileMapCountY)
    {    
        if (Coord->WorldTileY < World->CountY - 1)
        {
            Coord->WorldTileY++;
            Coord->Y = Coord->Y - World->TileHeight * World->TileMapCountY;
        }
        else
        {
            Result = false;
        }
    }

    return Result;
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
        GameState->PlayerWorldX = 0;
        GameState->PlayerWorldY = 0;
    }
    
    DrawRectangle(Buffer, 
            0.0f, 0.0f, 
            (real32)Buffer->Width, (real32)Buffer->Height,
            1.0f, 0.0f, 1.0f);

    world World = {};
    World.TileMapCountX = 16;
    World.TileMapCountY = 9;
    World.TileWidth = Buffer->Width / World.TileMapCountX;
    World.TileHeight = Buffer->Height / World.TileMapCountY;

    uint32 Tiles00[9][16] = 
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

    uint32 Tiles01[9][16] = 
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

    uint32 Tiles10[9][16] = 
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

    uint32 Tiles11[9][16] = 
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
    tile_map Tiles[2][2];
    Tiles[0][0].Map = (uint32 *)Tiles00;
    Tiles[0][1].Map = (uint32 *)Tiles01;
    Tiles[1][0].Map = (uint32 *)Tiles10;
    Tiles[1][1].Map = (uint32 *)Tiles11;

    World.TileMaps = (tile_map *)Tiles;
    World.CountX = 2;
    World.CountY = 2;
    

    game_controller_input Input0 = Input->Controllers[0];
    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerWidth = 0.5f * (real32)World.TileWidth;
    real32 PlayerHeight = (real32)World.TileHeight;
    real32 PlayerHeadOffset = 0.3f * (real32)World.TileHeight;

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

    if (NewPlayerX != GameState->PlayerX || NewPlayerY != GameState->PlayerY)
    {
        

        if (
            CanMove(&World, NewPlayerX,                          
                    (real32)(NewPlayerY + 0.5f * World.TileHeight), 
                    GameState->PlayerWorldX, GameState->PlayerWorldY) &&
            CanMove(&World, (real32)(NewPlayerX+ 0.5f * World.TileWidth), 
                    (real32)(NewPlayerY + 0.5 * World.TileHeight),
                    GameState->PlayerWorldX, GameState->PlayerWorldY) &&
            CanMove(&World, NewPlayerX,                          
                    (real32)(NewPlayerY + World.TileHeight),
                    GameState->PlayerWorldX, GameState->PlayerWorldY) &&
            CanMove(&World, (real32)(NewPlayerX + 0.5f * World.TileWidth), 
                    (real32)(NewPlayerY + World.TileHeight),
                    GameState->PlayerWorldX, GameState->PlayerWorldY))
        {
            world_coordibate Coord = {};
            Coord.X = NewPlayerX;
            Coord.Y = NewPlayerY;
            Coord.WorldTileX = GameState->PlayerWorldX;
            Coord.WorldTileY = GameState->PlayerWorldY;

            if (Advance(&World, &Coord))
            {
                GameState->PlayerY = Coord.Y;
                GameState->PlayerX = Coord.X;
                GameState->PlayerWorldX = Coord.WorldTileX;
                GameState->PlayerWorldY = Coord.WorldTileY;
            }
        }
        
    }
    
    tile_map *TileMap = GetTileMap(&World, GameState->PlayerWorldX, GameState->PlayerWorldY);
    for (int Row = 0; Row < World.TileMapCountY; Row++)
    {
        for (int Col = 0; Col < World.TileMapCountX; Col++)
        {
            
            DrawRectangle(Buffer, 
                (real32)Col * World.TileWidth, (real32)Row * World.TileHeight, 
                (real32)(Col +1) * World.TileWidth , (real32)(Row+1) * World.TileHeight,
                0.0f, (real32)TileMap->Map[Row * World.TileMapCountX + Col], 1.0f);
        }
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