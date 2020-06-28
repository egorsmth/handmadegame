
#include "handmade.h"

inline tile_chunk*
GetTileChunk(tile_map *TileMap, uint32 X, uint32 Y, uint32 Z,
            memory_arena *Arena = 0)
{
    uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
    uint32 HashIndex = 19*X + 7*Y + 3*Z;
    uint32 HashSlot = HashIndex & (ArrayCount(TileMap->TileChunkHash) - 1);
    tile_chunk *Chunk = &TileMap->TileChunkHash[HashSlot];
    do
    {
        if (
            X == Chunk->TileChunkX &&
            Y == Chunk->TileChunkY &&
            Z == Chunk->TileChunkZ)
        {
            break;
        }
        if (Arena && (Chunk->TileChunkX != 0) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, tile_chunk);
            Chunk->TileChunkX = 0;
            Chunk = Chunk->NextInHash;
        }

        if (Chunk->TileChunkX == 0 && Arena)
        {
            Chunk->TileChunkX = X;
            Chunk->TileChunkY = Y;
            Chunk->TileChunkZ = Z;
            Chunk->Map = PushArray(Arena, TileCount, uint32);
            for (uint32 TileIndex = 0; TileIndex < TileCount; TileIndex++)
            {
                Chunk->Map[TileIndex] = 1;
            }

            Chunk->NextInHash = 0;
            break;
        }

        Chunk = Chunk->NextInHash;
    } while (Chunk);
    
    return Chunk;
}

inline void 
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 X, uint32 Y, uint32 Val)
{
    Assert(TileChunk)
    Assert(X < TileMap->ChunkDim);
    Assert(Y < TileMap->ChunkDim);
    
    TileChunk->Map[Y*TileMap->ChunkDim + X] = Val;
}


inline uint32 
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 X, uint32 Y)
{
    Assert(TileChunk)
    Assert(X < TileMap->ChunkDim);
    Assert(Y < TileMap->ChunkDim);
    
    uint32 Value = TileChunk->Map[Y*TileMap->ChunkDim + X];
    return Value;
}

inline void 
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, 
            uint32 X, uint32 Y, uint32 Val)
{
    if (TileChunk)
    {
        SetTileValueUnchecked(TileMap, TileChunk, X, Y, Val);
    }   
}

inline uint32 
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 X, uint32 Y)
{
    uint32 Tile = 0;
    if (TileChunk)
    {
        Tile = GetTileValueUnchecked(TileMap, TileChunk, X, Y);
    }   
    return Tile;
}

inline tile_chunk_position 
GetChunkPosition(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;
    

    return Result;
}

internal uint32 
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position ChunkPos = GetChunkPosition(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ); 
    uint32 Value = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
    return Value;
}

internal uint32 
GetTileValue(tile_map *TileMap, tile_map_postition *Pos)
{
    uint32 Value = GetTileValue(TileMap, Pos->AbsTileX, Pos->AbsTileY, Pos->AbsTileZ);
    return Value;
}

internal bool 
IsTileValueEmpty(uint32 Tile)
{
    return Tile != 2;
}

bool 
CanMove(tile_map *TileMap, tile_map_postition *Pos)
{
    uint32 Tile = GetTileValue(TileMap, Pos);
    
    return IsTileValueEmpty(Tile);
}

inline void 
RecanonicalazeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel)
{
    int32 Offset = RoundReal32toInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= TileMap->TileSideInMeters * Offset;
    
    Assert(*TileRel >= -0.5f*TileMap->TileSideInMeters);
    Assert(*TileRel <= 0.5f*TileMap->TileSideInMeters);
}

// inline void RecanonicalizePostion(tile_map *World, tile_map_postition *Pos)
// {
//     RecanonicalazeCoord(World, 
//         &Pos->AbsTileX, &Pos->RelTile.X);
//     RecanonicalazeCoord(World, 
//         &Pos->AbsTileY, &Pos->RelTile.Y);
// }

inline tile_map_postition
MapIntoTileSpace(tile_map *TileMap, tile_map_postition BasePos, v2 Offset)
{
    tile_map_postition Result = BasePos;
    Result.RelTile += Offset;
    RecanonicalazeCoord(TileMap, 
        &Result.AbsTileX, &Result.RelTile.X);
    RecanonicalazeCoord(TileMap, 
        &Result.AbsTileY, &Result.RelTile.Y);
    return Result;
}

inline void 
SetTileValue(memory_arena *Arena, tile_map *TileMap, 
            uint32 X, uint32 Y, uint32 Z,
            uint32 Val)
{
    tile_chunk_position ChunkPos = GetChunkPosition(TileMap, X, Y, Z);
    tile_chunk *TileChunk = GetTileChunk(TileMap, 
        ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ,
        Arena); 
    Assert(TileChunk);

    if (!TileChunk->Map)
    {
        uint32 TileCount = TileMap->ChunkDim*TileMap->ChunkDim;
        TileChunk->Map = PushArray(Arena, TileCount, uint32);
        for (uint32 TileIndex = 0; TileIndex < TileCount; TileIndex++)
        {
            TileChunk->Map[TileIndex] = 1;
        }
    }

    SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, Val);
}

inline bool 
IsOnSameTile(tile_map_postition *Pos1, tile_map_postition *Pos2)
{
    bool Result = (Pos1->AbsTileX == Pos2->AbsTileX) &&
        (Pos1->AbsTileY == Pos2->AbsTileY) &&
        (Pos1->AbsTileZ == Pos2->AbsTileZ);
    return Result;
}

inline void 
UpdateCameraPos(tile_map *TileMap, tile_map_postition *Camera, tile_map_postition *Player)
{
    tile_chunk_position Pos = GetChunkPosition(TileMap, Player->AbsTileX, Player->AbsTileY, Player->AbsTileZ);
    int32 TileX = Pos.TileChunkX;
    int32 TileY = Pos.TileChunkY;
    int32 TileZ = Pos.TileChunkZ;

    TileX += 17 / 2;
    TileY += 9 / 2;

    Camera->AbsTileX = TileX;
    Camera->AbsTileY = TileY;
    Camera->AbsTileZ = TileZ;
}

inline v2 
Substract(tile_map *TileMap, tile_map_postition *A, tile_map_postition *B)
{
    v2 Diff = {};
    
    v2 DTile = V2((real32)A->AbsTileX - (real32)B->AbsTileX, (real32)A->AbsTileY - (real32)B->AbsTileY);

    Diff = TileMap->TileSideInMeters * DTile  + (A->RelTile - B->RelTile);
    return Diff;
}

inline tile_map_postition 
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_map_postition Result = {};

    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;

    return Result;
}

internal void
InitializeTileMap(tile_map *TileMap)
{
    TileMap->ChunkShift = 8;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = (1 << TileMap->ChunkShift);

    TileMap->TileSideInMeters = 1.4f;
    TileMap->TileSideInPixels = 60;
    TileMap->PixPerMeter = RoundReal32toInt32((real32)TileMap->TileSideInPixels / TileMap->TileSideInMeters);

    for (uint32 TileChunkIndex = 0; 
        TileChunkIndex < ArrayCount(TileMap->TileChunkHash); 
        TileChunkIndex++)
    {
        TileMap->TileChunkHash[TileChunkIndex].TileChunkX = 0;
    }
}