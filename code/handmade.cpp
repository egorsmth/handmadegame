#include "handmade.h"
#include "corecrt_math.h"
#include "tile.cpp"
#include "intrinsics.h"

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
                            v2 vMin,
                            v2 vMax,
                            real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32toInt32(vMin.X);
    int32 MaxX = RoundReal32toInt32(vMax.X);
    int32 MinY = RoundReal32toInt32(vMin.Y);
    int32 MaxY = RoundReal32toInt32(vMax.Y);
    
    
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
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzRezolution;
    int32 VertRezolution;
    uint32 ColorsUsed;
    uint32 ColorsImported;
    
    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
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

        uint32 RedMask = Header->RedMask;  
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedShift   = FindFirstSetBit(RedMask);
        bit_scan_result GreenShift = FindFirstSetBit(GreenMask);
        bit_scan_result BlueShift  = FindFirstSetBit(BlueMask);
        bit_scan_result AlphaShift = FindFirstSetBit(AlphaMask);
        
        // if big endian (not exactly)
        uint32 *SourceDest = Result;
        for (int32 Y = 0; Y < -Header->Height; Y++)
        {
            for (int32 X = 0; X < Header->Width; X++)
            {
                uint32 C = *SourceDest;
                *SourceDest++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
                                 (((C >> RedShift.Index) & 0xFF) << 16) |
                                 (((C >> GreenShift.Index) & 0xFF) << 8) |
                                 (((C >> BlueShift.Index) & 0xFF) << 0));
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

internal void DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, v2 Coord)
{
    int32 MinX = RoundReal32toInt32(Coord.X);
    int32 MaxX = RoundReal32toInt32(Coord.X + (real32)Bitmap->Width);
    int32 MinY = RoundReal32toInt32(Coord.Y);
    int32 MaxY = RoundReal32toInt32(Coord.Y + (real32)Bitmap->Height);
    int32 StartX = 0;
    int32 StartY = 0;
    int32 EndX = 0;
    int32 EndY = 0;
    if (MinX < 0)
    {
        StartX = -MinX;
        MinX = 0;
    }

    if (MinY < 0)
    {
        StartY = -MinY;
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

    uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1) - Bitmap->Width  * StartY + StartX;
    uint8 *DestRow = ((uint8 *)Buffer->Memory + 
                    MinX * Buffer->BytesPerPixel + 
                    MinY * Buffer->Pitch);
    for (int32 Y = MinY; Y < MaxY - EndY; Y++)
    {
            uint32 *Dest = (uint32 *)DestRow;
            uint32 *Source = SourceRow;
            for (int32 X = MinX; X < MaxX - EndX; X++)
            {
                real32 A =  (real32)((*Source >> 24) & 0xFF) / 255.0f;
                real32 SR = (real32)((*Source >> 16) & 0xFF);
                real32 SG = (real32)((*Source >> 8) & 0xFF);
                real32 SB = (real32)((*Source >> 0) & 0xFF);

                real32 DR = (real32)((*Dest >> 16) & 0xFF);
                real32 DG = (real32)((*Dest >> 8) & 0xFF);
                real32 DB = (real32)((*Dest >> 0) & 0xFF);

                real32 R = (1.0f - A)*DR + A * SR;
                real32 G = (1.0f - A)*DG + A * SG;
                real32 B = (1.0f - A)*DB + A * SB;

                *Dest = (
                    ((uint32)(R + 0.5f) << 16) |
                    ((uint32)(G + 0.5f) << 8) |
                    ((uint32)(B + 0.5f) << 0)
                );
                Dest++;
                Source++;
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
        
        GameState->HeroBitmap[0].Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/head_back.bmp");
        GameState->HeroBitmap[0].Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/body_back.bmp");
        GameState->HeroBitmap[0].Legs = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/legs_back.bmp");
        
        GameState->HeroBitmap[1].Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/head_right.bmp");
        GameState->HeroBitmap[1].Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/body_right.bmp");
        GameState->HeroBitmap[1].Legs = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/legs_right.bmp");

        GameState->HeroBitmap[2].Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/head_front.bmp");
        GameState->HeroBitmap[2].Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/body_front.bmp");
        GameState->HeroBitmap[2].Legs = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/legs_front.bmp");

        GameState->HeroBitmap[3].Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/head_left.bmp");
        GameState->HeroBitmap[3].Body = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/body_left.bmp");
        GameState->HeroBitmap[3].Legs = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/legs_left.bmp");

        GameState->PlayerP.RelTile = {};
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;

        GameState->CameraP.AbsTileX = 17 / 2;
        GameState->CameraP.AbsTileY = 9 / 2;
        GameState->CameraP.AbsTileZ = 0;

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

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;   

    game_controller_input Input0 = Input->Controllers[0];
    v2 ddPlayer = {};
    real32 Delta = 1.0f;    
    if (Input0.Up.EndedDown)
    {
        GameState->HeroFacingDirection = 0;
        ddPlayer.Y = Delta;
    }
    if (Input0.Down.EndedDown)
    {
        GameState->HeroFacingDirection = 2;
        ddPlayer.Y = -Delta;
    }
    if (Input0.Left.EndedDown)
    {
        GameState->HeroFacingDirection = 3;
        ddPlayer.X = -Delta;
    }
    if (Input0.Right.EndedDown)
    {
        GameState->HeroFacingDirection = 1;
        ddPlayer.X = Delta;
    }

    if (ddPlayer.X != 0.0f && ddPlayer.Y != 0.0f)
    {       
        ddPlayer *= 0.707106781187f;
    }
    real32 PlayerSpeed = 10.0f;
    ddPlayer *= PlayerSpeed;

    // ODE should be here
    ddPlayer += -2.0f*GameState->dPlayer;
    
    v2 NewPlayerPos = GameState->PlayerP.RelTile;
    NewPlayerPos = 0.5f * ddPlayer * powf((real32)Input->SecondsToAdvance, 2.0f)
                 + GameState->dPlayer * (real32)Input->SecondsToAdvance
                 + NewPlayerPos; // new_p = 0.5*a*t^2 + v*t + p
    GameState->dPlayer = ddPlayer * (real32)Input->SecondsToAdvance + GameState->dPlayer; // v = a*t + p
    if (NewPlayerPos.X != GameState->PlayerP.RelTile.X || NewPlayerPos.Y != GameState->PlayerP.RelTile.Y)
    {
        #if 0
        tile_map_postition BottomLeft = GameState->PlayerP;
        BottomLeft.RelTile.X = NewPlayerPos.X - 0.5f*PlayerWidth;
        BottomLeft.RelTile.Y = NewPlayerPos.Y;
        tile_map_postition BottomRight = BottomLeft;
        BottomRight.RelTile.X += PlayerWidth;
        RecanonicalizePostion(TileMap, &BottomLeft);
        RecanonicalizePostion(TileMap, &BottomRight);
        if (
            CanMove(TileMap, &BottomLeft) &&
            CanMove(TileMap, &BottomRight))
        {
            tile_map_postition TopRight = GameState->PlayerP;
            TopRight.RelTile = NewPlayerPos;
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
        else
        {
            // TODO: this simulation of only bottom collision,
            // need to implement collision from any side
            v2 r = {0.0f, 1.0f};
            GameState->dPlayer = GameState->dPlayer - 2*Inner(GameState->dPlayer, r) * r;
        }
        #else
        tile_map_postition BottomLeft = GameState->PlayerP;
        BottomLeft.RelTile.X = NewPlayerPos.X - 0.5f*PlayerWidth;
        BottomLeft.RelTile.Y = NewPlayerPos.Y;

        tile_map_postition BestPlayerP = GameState->PlayerP;
        real32 BestDistanceSq = LenghtSq(PlayerDelta);
        for (uint32 AnsTileX = MinTileX; AnsTileX != OnePastTileX; AnsTileX++)
        {
            for (uint32 AnsTileY = MinTileX; AnsTileY != OnePastTileY; AnsTileY++)
            {
                tile_map_postition Pos = {};
                uint32 TileValue = GetTileValue(TileMap, &Pos);
                if (IsTileEmpty(TileValue))
                {
                    v2 MinCorner = ;
                    v2 MaxCorner = ;
                    
                    v2 RelNewPlayerP = Substract(TileMap, &Pos, &BottomLeft);
                    v2 TestP = ClosesPointInRectangle(MinCorner, MaxCorner, RelNewPlayerP);
                    real32 TestDistanceSq = LengthSq(TestP);
                    if (BestDistanceSq < TestDistanceSq)
                    {
                        BestPlayerP = TestP;
                        BestDistanceSq = TestDistanceSq;
                    }
                }
            }
        }
        
        #endif
        v2 Diff = Substract(TileMap, &GameState->PlayerP, &GameState->CameraP);
        if (Diff.X > 9.0 * TileMap->TileSideInMeters)
        {
            GameState->CameraP.AbsTileX += 17;
        }
        if (Diff.X < -9.0 * TileMap->TileSideInMeters)
        {
            GameState->CameraP.AbsTileX -= 17;
        }
        if (Diff.Y > 5.0 * TileMap->TileSideInMeters)
        {
            GameState->CameraP.AbsTileY += 9;
        }
        if (Diff.Y < -5.0 * TileMap->TileSideInMeters)
        {
            GameState->CameraP.AbsTileY -= 9;
        }

        GameState->CameraP.AbsTileZ = GameState->PlayerP.AbsTileZ;
    }
    
    DrawBitmap(Buffer, &GameState->Backdrop, V2(0,0));

    v2 ViewCenter = V2((real32)Buffer->Width / 2, (real32)Buffer->Height / 2);
    real32 Red = 0.0f;
    real32 Green = 0.0f;
    real32 Blue = 0.0f;
    for (int32 RelRow = -10; RelRow <  10; RelRow++)
    {
        for (int32 RelCol = - 20; RelCol < 20; RelCol++)
        {
            uint32 Col = RelCol + GameState->CameraP.AbsTileX;
            uint32 Row = RelRow + GameState->CameraP.AbsTileY;
            
            v2 tileCenter = V2(
                ViewCenter.X - (real32)TileMap->PixPerMeter * GameState->CameraP.RelTile.X + ((real32)RelCol)*TileMap->TileSideInPixels, 
                ViewCenter.Y + (real32)TileMap->PixPerMeter * GameState->CameraP.RelTile.Y - ((real32)RelRow)*TileMap->TileSideInPixels
            );
            v2 TileSide = V2(0.5f * TileMap->TileSideInPixels, 0.5f * TileMap->TileSideInPixels);
            v2 Min = tileCenter - TileSide;
            v2 Max = tileCenter + TileSide;
            uint32 Val = GetTileValue(TileMap, Col, Row, GameState->CameraP.AbsTileZ);
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
                Min, Max,
                Red, Green, Blue);
        }
    }

    v2 Diff = Substract(TileMap, &GameState->PlayerP, &GameState->CameraP);
    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    v2 PlayerTopLeft = V2(
        ViewCenter.X + (real32)TileMap->PixPerMeter * Diff.X - PlayerWidth*TileMap->PixPerMeter*0.5f,
        ViewCenter.Y - (real32)TileMap->PixPerMeter * Diff.Y - PlayerHeight*TileMap->PixPerMeter
    );
    v2 PixPerMeter = V2(PlayerWidth*TileMap->PixPerMeter, PlayerHeight*TileMap->PixPerMeter);
    DrawRectangle(Buffer, PlayerTopLeft, 
                PlayerTopLeft + PixPerMeter,
                PlayerR, PlayerG, PlayerB);
    hero_bitmaps HeroBitmaps = GameState->HeroBitmap[GameState->HeroFacingDirection];
    DrawBitmap(Buffer, &HeroBitmaps.Legs, PlayerTopLeft);
    DrawBitmap(Buffer, &HeroBitmaps.Body, PlayerTopLeft);
    DrawBitmap(Buffer, &HeroBitmaps.Head, PlayerTopLeft);

   // DrawPoint(Buffer, PlayerLeft + 1, PlayerTop + 1, 1.0, 0.0, 0.0);
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