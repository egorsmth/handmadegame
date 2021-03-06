#include "handmade.h"
#include "corecrt_math.h"
#include "world.cpp"
#include "intrinsics.h"

#include <cstdlib>

internal void 
DrawPoint(game_offscreen_buffer *Buffer,
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

internal void 
DrawRectangle(game_offscreen_buffer *Buffer,
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

internal loaded_bitmap 
DEBUGLoadBMP(
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

internal void 
InitializeArena(memory_arena *Arena,
                memory_index Size, uint8 *Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

internal void 
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, v2 Coord)
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

internal bool 
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 *tMin, real32 MinY, real32 MaxY)
{
    bool Collided = false;
    real32 tEpsilon = 0.001f;
    if(PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        real32 Y = RelY + tResult*PlayerDeltaY;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY))
            {
                Collided = true;
                *tMin = Maximum(0.0f, tResult - tEpsilon);
            }
        }
    }
    return Collided;
}

bool 
hasHotEntity(game_state *GameState, uint32 ColdIndex)
{
    if (
        (
            ColdIndex == GameState->PlayerEntityIndex && 
            GameState->HotEntityCount > 0
        ) || 
        GameState->ColdEntity[ColdIndex].HotIndex)
    {
        return true;
    }
    return false;
}

internal entity
GetEntity(game_state *GameState, uint32 idx)
{
    entity Entity = {};
    if (idx < GameState->ColdEntityCount)
    {
        Entity.Cold = &GameState->ColdEntity[idx];
        if (hasHotEntity(GameState, idx))
        {
            Entity.Hot = &GameState->HotEntity[Entity.Cold->HotIndex];
        }
    }
    
    return Entity;
}

internal uint32
AddEntity(game_state *GameState, world_postition P)
{
    uint32 idx = GameState->ColdEntityCount;
    GameState->ColdEntity[idx] = {};
    GameState->ColdEntity[idx].P = P;

    GameState->ColdEntityCount++;
    ChangeEntityLocation(&GameState->WorldArena, GameState->World, idx, 0, &P);
    return idx;
}

inline v2 
GetCameraSpace(game_state *GameState, cold_entity *Cold)
{
    v2 Diff = Substract(
        GameState->World, 
        &GameState->CameraP, 
        &Cold->P);
    return Diff;
}

inline hot_entity *
MakeEntityHot(game_state *GameState, uint32 ColdIdx, v2 P)
{
    Assert(!hasHotEntity(GameState, ColdIdx) && GameState->HotEntityCount < ArrayCount(GameState->HotEntity));
    cold_entity *Cold = &GameState->ColdEntity[ColdIdx];
    hot_entity *Result = 0;
    uint32 HotIndex = GameState->HotEntityCount++;
    Result = &GameState->HotEntity[HotIndex];
    Cold->HotIndex = HotIndex;
    Result->P = P;
    Result->dP = {0, 0};
    Result->ChunkZ = Cold->P.ChunkZ;
    Result->ColdIndex = ColdIdx;
    return Result;
}

inline hot_entity *
MakeEntityHot(game_state *GameState, uint32 ColdIdx)
{
    hot_entity *Result = 0;
    cold_entity *Cold = &GameState->ColdEntity[ColdIdx];
    if (hasHotEntity(GameState, ColdIdx) && GameState->HotEntityCount < ArrayCount(GameState->HotEntity))
    {
        Result = &GameState->HotEntity[Cold->HotIndex];
    }
    else
    {
        v2 CameraSpace = GetCameraSpace(GameState, Cold);
        Result = MakeEntityHot(GameState, ColdIdx, CameraSpace);
    }
    return Result;
}

internal void
MakeEntityCold(game_state *GameState, uint32 ColdIdx)
{
    cold_entity *Cold = &GameState->ColdEntity[ColdIdx];
    if (hasHotEntity(GameState, ColdIdx))
    {
        if (Cold->HotIndex == GameState->HotEntityCount - 1)
        {
            --GameState->HotEntityCount;
        }
        else
        {
            hot_entity *Exchanger = &GameState->HotEntity[GameState->HotEntityCount - 1];
            GameState->HotEntity[Cold->HotIndex] = *Exchanger;
            GameState->ColdEntity[Exchanger->ColdIndex].HotIndex = Cold->HotIndex;
            GameState->HotEntityCount--;
            Cold->HotIndex = NULL;
        }
    }
}

internal void
InitializePlayer(game_state *GameState)
{
    //world_postition P = MapIntoChunkSpace(GameState->World, GameState->CameraP, V2(0,0));
    uint32 idx = AddEntity(GameState, GameState->CameraP);
    GameState->PlayerEntityIndex = idx;
    entity Entity = GetEntity(GameState, idx);
    Entity.Cold->Width = 0.7f;
    Entity.Cold->Height = 0.4f;
    Entity.Cold->Collides = true;
    Entity.Cold->Type = EntityType_Player;
    MakeEntityHot(GameState, GameState->PlayerEntityIndex);
}

internal uint32
AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_postition P = ChunkPositionFromTilePOstition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    //P = MapIntoChunkSpace(GameState->World, P, V2(0,0));
    uint32 idx = AddEntity(GameState, P);
    entity Entity = GetEntity(GameState, idx);
    Entity.Cold->Width = GameState->World->TileSideInMeters;
    Entity.Cold->Height = GameState->World->TileSideInMeters;
    Entity.Cold->Collides = true;
    Entity.Cold->Type = EntityType_Wall;
    return idx;
}

internal uint32
AddMonster(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_postition P = ChunkPositionFromTilePOstition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    //P = MapIntoChunkSpace(GameState->World, P, V2(0,0));
    uint32 idx = AddEntity(GameState, P);
    entity Entity = GetEntity(GameState, idx);
    Entity.Cold->Width = 0.7f;
    Entity.Cold->Height = 0.4f;
    Entity.Cold->Collides = true;
    Entity.Cold->Type = EntityType_Monster;
    return idx;
}

internal uint32
AddFamiliar(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_postition P = ChunkPositionFromTilePOstition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    //P = MapIntoChunkSpace(GameState->World, P, V2(0,0));
    uint32 idx = AddEntity(GameState, P);
    entity Entity = GetEntity(GameState, idx);
    Entity.Cold->Width = 0.7f;
    Entity.Cold->Height = 0.4f;
    Entity.Cold->Collides = false;
    Entity.Cold->Type = EntityType_Familiar;
    return idx;
}

inline bool
ValidateEntityPairs(game_state *GameState)
{
    bool Valid = true;
    for (uint32 EntityIndex = 0; Valid && EntityIndex < GameState->HotEntityCount; EntityIndex++)
    {
        uint32 ColdIdx = GameState->HotEntity[EntityIndex].ColdIndex;
        Valid = GameState->ColdEntity[ColdIdx].HotIndex == EntityIndex;
    }
    return Valid;
}

inline void
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 Bounds)
{
    // do not check player (player cold index = 0)
	for(uint32 EntityIndex = 1;EntityIndex < GameState->HotEntityCount;)
	{
		hot_entity *Hot = &GameState->HotEntity[EntityIndex];
        Assert(GameState->ColdEntity[Hot->ColdIndex].HotIndex == EntityIndex);
		Hot->P += Offset;
		if(IsInRectangle(Bounds, Hot->P))
		{
			++EntityIndex;
		}
		else
		{
			MakeEntityCold(GameState, Hot->ColdIndex);
		}
	}
}

internal void
SetCamera(game_state *GameState, world_postition NewCameraP)
{
    Assert(ValidateEntityPairs(GameState));
    v2 Diff = Substract(GameState->World, &NewCameraP, &GameState->CameraP);
    GameState->CameraP = NewCameraP;
    
    uint32 TileXSpan = 17*3;
    uint32 TileYSpan = 9*3;
    rectangle2 CameraBounds = RectCenterDim(
        V2(0,0),
        GameState->World->TileSideInMeters*V2((real32)TileXSpan, (real32)TileYSpan));
    OffsetAndCheckFrequencyByArea(GameState, -Diff, CameraBounds);
    Assert(ValidateEntityPairs(GameState));
    world_postition MinChunkP = MapIntoChunkSpace(GameState->World, NewCameraP, GetMinCorner(CameraBounds));
    world_postition MaxChunkP = MapIntoChunkSpace(GameState->World, NewCameraP, GetMaxCorner(CameraBounds));
    for (int32 ChunkY = MinChunkP.ChunkY;ChunkY <= MaxChunkP.ChunkY; ChunkY++)
    {
        for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ChunkX++)
        {
            world_chunk *Chunk = GetWorldChunk(GameState->World, ChunkX, ChunkY, NewCameraP.ChunkZ);
            if (Chunk)
            {
                for (world_entity_block *Block = &Chunk->FirstBlock;Block;Block=Block->Next)
                {
                    for (uint32 EntityIndex = 0; EntityIndex < Block->EntitiesCount; EntityIndex++)
                    {
                        if (!hasHotEntity(GameState, Block->LowEntityIndex[EntityIndex]))
                        {
                            cold_entity *Cold = &GameState->ColdEntity[Block->LowEntityIndex[EntityIndex]];
                            v2 CameraSpace = GetCameraSpace(GameState, Cold);
                            if (IsInRectangle(CameraBounds, CameraSpace))
                            {
                                MakeEntityHot(GameState, Block->LowEntityIndex[EntityIndex], CameraSpace);
                            }
                        }
                    }
                }
            }
        }
    }
    Assert(ValidateEntityPairs(GameState));
}

// internal void
// UpdateEntity(GameState, Entity, dt)
// {

// }

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
        
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {
        
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

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        InitializeTileMap(GameState->World);
        
        real32 TileSideInPixels = 60.0f;
        real32 LowerLeftX = -TileSideInPixels/2;
        real32 LowerLeftY = (real32)Buffer->Height;
        uint32 TilePerWidth = 17;
        uint32 TilePerHeight = 9;
        uint32 ScreenBaseX = (INT16_MAX / TilePerWidth) / 2;
        uint32 ScreenBaseY = (INT16_MAX / TilePerHeight) / 2;
        uint32 ScreenBaseZ =  INT16_MAX / 2;
        uint32 ScreenX = ScreenBaseX;
        uint32 ScreenY = ScreenBaseY;
        uint32 ScreenZ = ScreenBaseZ;
        bool DoorUp = false;
        bool DoorDown = false;
        bool DoorStarted = false;
        world_postition NewCameraP = {};
        uint32 CameraX = ScreenBaseX * TilePerWidth + 17 / 2;
        uint32 CameraY = ScreenBaseY * TilePerHeight + 9 / 2;
        uint32 CameraZ = ScreenBaseZ;
        NewCameraP = ChunkPositionFromTilePOstition(GameState->World, 
            CameraX, CameraY, CameraZ);
        GameState->CameraP = NewCameraP;
        InitializePlayer(GameState); // initiliztion should be before walls becous index 0
        // in cold entitiyes should be hero, fix that some day, make 0 index forever empty and start from 1
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

                    if (!IsTileValueEmpty(Val))
                    {
                        AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
                    }
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
        
        AddMonster(GameState, CameraX + 2, CameraY + 2, CameraZ);
        AddFamiliar(GameState, CameraX - 2, CameraY + 2, CameraZ);
        SetCamera(GameState, NewCameraP);
        Memory->isInitialized = true;
    }

    game_controller_input Input0 = Input->Controllers[0];
    world *World = GameState->World;   

    v2 ddPlayer = {};
    real32 Delta = 1.0f;
    entity Player = GetEntity(GameState, GameState->PlayerEntityIndex);
    if (Input0.Up.EndedDown)
    {
        Player.Hot->FacingDirection = 0;
        ddPlayer.Y = Delta;
    }
    if (Input0.Down.EndedDown)
    {
        Player.Hot->FacingDirection = 2;
        ddPlayer.Y = -Delta;
    }
    if (Input0.Left.EndedDown)
    {
        Player.Hot->FacingDirection = 3;
        ddPlayer.X = -Delta;
    }
    if (Input0.Right.EndedDown)
    {
        Player.Hot->FacingDirection = 1;
        ddPlayer.X = Delta;
    }

    real32 ddPLength = LengthSq(ddPlayer);
    if (ddPLength > 1.0f)
    {       
        ddPlayer *= (1.0f / sqrtf(ddPLength));
    }
    real32 PlayerSpeed = 10.0f;
    ddPlayer *= PlayerSpeed;

    // ODE should be here
    ddPlayer += -8.0f * Player.Hot->dP;
    real32 dt = (real32)Input->SecondsToAdvance;
    v2 OldPlayerP = Player.Hot->P;
    v2 PlayerDelta = 0.5f * ddPlayer * powf(dt, 2.0f)
                 + Player.Hot->dP * dt;
    v2 NewPlayerP = OldPlayerP + PlayerDelta; // new_p = 0.5*a*t^2 + v*t + p

    if (NewPlayerP.X != OldPlayerP.X
     || NewPlayerP.Y != OldPlayerP.Y)
    {
        real32 tMin = 1.0f;
        uint32 HitEntityIndex = 0;

        for (uint32 EntityIndex = 0; 
            EntityIndex < GameState->HotEntityCount; 
            EntityIndex++)
        {
            entity TestEntity = GetEntity(GameState, GameState->HotEntity[EntityIndex].ColdIndex);

            if (TestEntity.Hot->ColdIndex == GameState->PlayerEntityIndex)
            {
                continue;
            }
            
            if (TestEntity.Cold->Collides)
            {
                real32 DiameterW = TestEntity.Cold->Width + Player.Cold->Width;
                real32 DiameterH = TestEntity.Cold->Height + Player.Cold->Height;
                v2 MinCorner = -0.5f * V2(DiameterW, DiameterH);
                v2 MaxCorner =  0.5f * V2(DiameterW, DiameterH);

                v2 Rel = Player.Hot->P - TestEntity.Hot->P;

                if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                        &tMin, MinCorner.Y, MaxCorner.Y))
                {
                    HitEntityIndex = EntityIndex;
                }
                if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                        &tMin, MinCorner.Y, MaxCorner.Y))
                {
                    HitEntityIndex = EntityIndex;
                }
                if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                        &tMin, MinCorner.X, MaxCorner.X))
                {
                    HitEntityIndex = EntityIndex;
                }
                if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                        &tMin, MinCorner.X, MaxCorner.X))
                {
                    HitEntityIndex = EntityIndex;
                }
            }
        }

        Player.Hot->dP = ddPlayer * dt + Player.Hot->dP; // v = a*t + p

        if (tMin < 1.0f)
        {
            Player.Hot->dP = V2(0,0);
        }
        
        NewPlayerP = OldPlayerP + tMin*PlayerDelta;

        if (HitEntityIndex)
        {
            entity HitEntity = GetEntity(GameState, HitEntityIndex);
            Player.Hot->ChunkZ += HitEntity.Cold->dAbsTileZ;
        }
        
        Player.Hot->P = NewPlayerP;
        world_postition NewWorldP = MapIntoChunkSpace(World, GameState->CameraP, Player.Hot->P);
        ChangeEntityLocation(&GameState->WorldArena, GameState->World, Player.Hot->ColdIndex, &Player.Cold->P, &NewWorldP);
        Player.Cold->P = NewWorldP;
#if 0
        world_postition NewCameraP = GameState->CameraP;
        if (Player.Hot->P.X > 9.0 * World->TileSideInMeters)
        {
           NewCameraP.ChunkX += 17;
        }
        if (Player.Hot->P.X < -9.0 * World->TileSideInMeters)
        {
            NewCameraP.ChunkX -= 17;
        }
        if (Player.Hot->P.Y > 5.0 * World->TileSideInMeters)
        {
            NewCameraP.ChunkY += 9;
        }
        if (Player.Hot->P.Y < -5.0 * World->TileSideInMeters)
        {
            NewCameraP.ChunkY -= 9;
        }
        NewCameraP.ChunkZ = Player.Hot->ChunkZ;
#else
        world_postition NewCameraP = Player.Cold->P;
#endif
        SetCamera(GameState, NewCameraP);
    }
    
    DrawBitmap(Buffer, &GameState->Backdrop, V2(0,0));

    v2 ViewCenter = V2((real32)Buffer->Width / 2, (real32)Buffer->Height / 2);

    real32 PixelPerMeter = 60.0f / World->TileSideInMeters;
    
    for (uint32 i = 0; i < GameState->HotEntityCount; i++)
    {
        entity Entity = GetEntity(GameState, GameState->HotEntity[i].ColdIndex);
        v2 EntityTopLeft = V2(
            ViewCenter.X 
            + PixelPerMeter * Entity.Hot->P.X 
            - 0.5f*Entity.Cold->Width * PixelPerMeter,
            ViewCenter.Y 
            - PixelPerMeter * Entity.Hot->P.Y 
            - 0.5f*Entity.Cold->Height * PixelPerMeter
        );
        v2 PixPerMeter = V2(
            Entity.Cold->Width * PixelPerMeter, 
            Entity.Cold->Height * PixelPerMeter);
        v2 EntityBottomRight = EntityTopLeft + PixPerMeter;
       // UpdateEntity(GameState, Entity, dt);
        if (Entity.Cold->Type == EntityType_Player)
        {
            real32 PlayerR = 1.0f;
            real32 PlayerG = 1.0f;
            real32 PlayerB = 0.0f;
            DrawRectangle(Buffer, EntityTopLeft, 
                        EntityBottomRight,
                        PlayerR, PlayerG, PlayerB);
            hero_bitmaps HeroBitmaps = GameState->HeroBitmap[Entity.Hot->FacingDirection];
            v2 PlayerBitmapTopLeft = EntityTopLeft + V2(
                -0.4f*(real32)HeroBitmaps.Body.Width,
                -0.8f*(real32)HeroBitmaps.Body.Height);
            DrawBitmap(Buffer, &HeroBitmaps.Legs, PlayerBitmapTopLeft);
            DrawBitmap(Buffer, &HeroBitmaps.Body, PlayerBitmapTopLeft);
            DrawBitmap(Buffer, &HeroBitmaps.Head, PlayerBitmapTopLeft);
        }
        if (Entity.Cold->Type == EntityType_Wall)
        {
            real32 WallR = 1.0f;
            real32 WallG = 0.0f;
            real32 WallB = 1.0f;
            DrawRectangle(Buffer, EntityTopLeft, 
                        EntityBottomRight,
                        WallR, WallG, WallB);
        }
        if (Entity.Cold->Type == EntityType_Monster)
        {
            real32 WallR = 1.0f;
            real32 WallG = 0.5f;
            real32 WallB = 1.0f;
            DrawRectangle(Buffer, EntityTopLeft, 
                        EntityBottomRight,
                        WallR, WallG, WallB);
        }
        if (Entity.Cold->Type == EntityType_Familiar)
        {
            real32 WallR = 1.0f;
            real32 WallG = 0.5f;
            real32 WallB = 0.3f;
            DrawRectangle(Buffer, EntityTopLeft, 
                        EntityBottomRight,
                        WallR, WallG, WallB);
        }
    }

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