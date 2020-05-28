#include "handmade.h"
#include "corecrt_math.h"
#include "tile.cpp"

#include <cstdlib>

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

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitMapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitesPerPixel;
};
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP(
    thread_context *Thread,
    debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
    loaded_bitmap Bitmap = {};
    
    debug_file_read FileResult = ReadEntireFile(Thread, Filename);
    if(FileResult.Content)
    {
        bitmap_header *Header = (bitmap_header *)FileResult.Content;
        uint32 *Result = 0;
        Result = (uint32 *)((uint8 *)FileResult.Content + Header->BitMapOffset);
        Bitmap.Width = Header->Width;
        Bitmap.Height = Header->Height;
        Bitmap.Pixels = Result;
        // if big endian (not exactly)
        uint32 *SourceDest = Result;
        for (int32 Y = 0; Y < Header->Width; Y++)
        {
            for (int32 X = 0; X < Header->Height; X++)
            {
                *SourceDest = (*SourceDest >> 8) | (*SourceDest << 24);
                ++SourceDest;
            }
        }
    }
    return Bitmap;
}

internal void InitializeArena(memory_arena *Arena,
                memory_index Size, uint8 *Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

internal void DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealX, real32 RealY)
{
    int32 MinX = RoundReal32toInt32(RealX);
    int32 MaxX = RoundReal32toInt32(RealX + (real32)Bitmap->Width);
    int32 MinY = RoundReal32toInt32(RealY);
    int32 MaxY = RoundReal32toInt32(RealY + (real32)Bitmap->Height);
    
    if (MinX < 0)
    {
        MinX = 0;
    }

    if (MinY < 0)
    {
        MinY = 0;
    }

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
    uint8 *DestRow = ((uint8 *)Buffer->Memory + 
                    MinX * Buffer->BytesPerPixel + 
                    MinY * Buffer->Pitch);
    for (int32 Y = MinY; Y < MaxY; Y++)
    {
            uint32 *Dest = (uint32 *)DestRow;
            uint32 *Source = SourceRow;
            for (int32 X = MinX; X < MaxX; X++)
            {
                *Dest++ = *Source++;
            }
            DestRow += Buffer->Pitch;
            SourceRow -= Bitmap->Width;
    }
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
        GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/background.bmp");
        GameState->PlayerHead = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/head.bmp");
        GameState->PlayerTorso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/torso.bmp");
        GameState->PlayerLegs = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/legs.bmp");
        GameState->PlayerP.RelTileX = 0.0f;
        GameState->PlayerP.RelTileY = 0.0f;
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);;


        tile_map *TileMap = World->TileMap;
        
        TileMap->ChunkShift = 8;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);
        TileMap->TileSideInMeters = 1.4f;
        TileMap->TileSideInPixels = 60;
        TileMap->PixPerMeter = RoundReal32toInt32((real32)TileMap->TileSideInPixels / TileMap->TileSideInMeters);
        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;
        TileMap->TileChunkCountZ = 2;
        TileMap->TileChunks = 
            PushArray(&GameState->WorldArena, 
                TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ,
                tile_chunk);
        
        // for (uint32 Y = 0 ; Y < TileMap->TileChunkCountY; Y++)
        // {
        //     for (uint32 X = 0 ; X < TileMap->TileChunkCountX; X++)
        //     {
        //         TileMap->TileChunks[Y*TileMap->TileChunkCountX + X].Map = 
        //                 PushArray(&GameState->WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
        //     }
        // }

        real32 LowerLeftX = -(real32)TileMap->TileSideInPixels/2;
        real32 LowerLeftY = (real32)Buffer->Height;
        uint32 TilePerWidth = 17;
        uint32 TilePerHeight = 9;
        uint32 ScreenY = 0;
        uint32 ScreenX = 0;
        uint32 ScreenZ = 0;
        bool DoorUp = true;
        bool DoorDown = false;
        bool DoorStarted = false;
        for (int32 ScreenIndex = 0; ScreenIndex < 100; ScreenIndex++)
        {
                for (uint32 TileY = 0; TileY < TilePerHeight; TileY++)
                {
                    for (uint32 TileX = 0; TileX < TilePerWidth; TileX++)
                    {
                        uint32 AbsTileX = ScreenX * TilePerWidth + TileX;
                        uint32 AbsTileY = ScreenY * TilePerHeight + TileY;
                        uint32 AbsTileZ = ScreenZ;
                        uint32 Val = 1;
                        if (TileX == 0 || TileX == TilePerWidth - 1)
                        {
                            Val = 2;
                        }
                        if (TileY == 0 || TileY == TilePerHeight - 1)
                        {
                            Val = 2;
                        }
                        if (TileX == 8 || TileY == 4)
                        {
                            Val = 1;
                        }
                        if (TileX == 10 && TileY == 6 && ScreenX == 0 && ScreenY == 0)
                        {
                            DoorStarted = !DoorStarted;
                            if (DoorUp)
                            {
                                Val = 3;
                            }

                            if (DoorDown)
                            {
                                Val = 4;
                            }
                        }
                        SetTileValue(&GameState->WorldArena, 
                                    World->TileMap, 
                                    AbsTileX, AbsTileY, AbsTileZ, Val);
                    }
                }
                
                if (DoorStarted)
                {
                    if (DoorUp)
                    {
                        ScreenZ++;
                    }
                    else if (DoorDown)
                    {
                        ScreenZ--;
                    }
                }
                else
                {
                    int random_variable = std::rand();
                    if (random_variable % 2 == 1)
                    {
                        ScreenX++;
                    }
                    else
                    {
                        ScreenY++;
                    }
                }
                if (DoorUp)
                {
                    DoorDown = true;
                    DoorUp = false;
                }
                else if (DoorDown)
                {
                    DoorUp = true;
                    DoorDown = false;
                }
                else
                {
                    DoorUp = false;
                    DoorDown = false;

                    
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

            if (!IsOnSameTile(&GameState->PlayerP, &TopRight))
            {
                uint32 TileValue = GetTileValue(TileMap, &TopRight);
                if (TileValue == 3)
                {
                    TopRight.AbsTileZ++;
                }
                else if (TileValue == 4)
                {
                    TopRight.AbsTileZ--;
                }
            }
            GameState->PlayerP = TopRight;
        }
    }
    
    DrawBitmap(Buffer, &GameState->Backdrop, 0.0f, 0.0f);

    real32 CenterX = (real32)Buffer->Width / 2;
    real32 CenterY = (real32)Buffer->Height / 2;
    real32 Red = 0.0f;
    real32 Green = 0.0f;
    real32 Blue = 0.0f;
    for (int32 RelRow = -10; RelRow <  10; RelRow++)
    {
        for (int32 RelCol = - 20; RelCol < 20; RelCol++)
        {
            uint32 Col = RelCol + GameState->PlayerP.AbsTileX;
            uint32 Row = RelRow + GameState->PlayerP.AbsTileY;
            
            real32 tileCenterX = CenterX - TileMap->PixPerMeter * GameState->PlayerP.RelTileX + ((real32)RelCol)*TileMap->TileSideInPixels;
            real32 tileCenterY = CenterY + TileMap->PixPerMeter * GameState->PlayerP.RelTileY - ((real32)RelRow)*TileMap->TileSideInPixels;
            real32 MinX = tileCenterX - 0.5f * TileMap->TileSideInPixels;
            real32 MinY = tileCenterY - 0.5f * TileMap->TileSideInPixels;
            real32 MaxX = tileCenterX + 0.5f * TileMap->TileSideInPixels;
            real32 MaxY = tileCenterY + 0.5f * TileMap->TileSideInPixels;
            uint32 Val = GetTileValue(TileMap, Col, Row, GameState->PlayerP.AbsTileZ);
            if (Val == 0)
            {
                Red = 1.0f;
                Green = 0.0f;
                Blue = 0.0f;
                continue;
            }
            else if (Val == 1)
            {
                Red = 0.5f;
                Green = 0.5f;
                Blue = 0.5f;
                continue;
            }
            else if (Val == 2)
            {
                Red = 1.0f;
                Green = 1.0f;
                Blue = 1.0f;
            }
            else if (Val == 3)
            {
                Red = 1.0f;
                Green = 0.5f;
                Blue = 0.5f;
            }
            else if (Val == 4)
            {
                Red = 0.5f;
                Green = 1.0f;
                Blue = 0.5f;
            }
            if (Row == GameState->PlayerP.AbsTileY && Col == GameState->PlayerP.AbsTileX)
            {
                Red = 0.0f;
                Green = 0.0f;
                Blue = 0.0f;
            }

            DrawRectangle(Buffer, 
                MinX, MinY, MaxX, MaxY,
                Red, Green, Blue);
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
    DrawBitmap(Buffer, &GameState->PlayerHead, PlayerLeft, PlayerTop);

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