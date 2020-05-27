
#include "handmade.h"

inline tile_chunk *GetTileChunk(tile_map *TileMap, uint32 X, uint32 Y, uint32 Z)
{
    tile_chunk* TileChunk = 0;
    if (X >= 0 && X < TileMap->TileChunkCountX && 
        Y >= 0 && Y < TileMap->TileChunkCountY &&
        Z >= 0 && Z < TileMap->TileChunkCountZ)
    {
        TileChunk = &TileMap->TileChunks[
            Z * TileMap->TileChunkCountY * TileMap->TileChunkCountX +
            Y * TileMap->TileChunkCountX +
            X
        ];
    }
    
    return TileChunk;
}

inline void SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 X, uint32 Y, uint32 Val)
{
    Assert(TileChunk)
    Assert(X < TileMap->ChunkDim);
    Assert(Y < TileMap->ChunkDim);
    
    TileChunk->Map[Y*TileMap->ChunkDim + X] = Val;
}


inline uint32 GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 X, uint32 Y)
{
    Assert(TileChunk)
    Assert(X < TileMap->ChunkDim);
    Assert(Y < TileMap->ChunkDim);
    
    uint32 Value = TileChunk->Map[Y*TileMap->ChunkDim + X];
    return Value;
}

inline void SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, 
            uint32 X, uint32 Y, uint32 Val)
{
    if (TileChunk)
    {
        SetTileValueUnchecked(TileMap, TileChunk, X, Y, Val);
    }   
}

inline uint32 GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 X, uint32 Y)
{
    uint32 Tile = 0;
    if (TileChunk)
    {
        Tile = GetTileValueUnchecked(TileMap, TileChunk, X, Y);
    }   
    return Tile;
}

inline tile_chunk_position GetChunkPosition(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;
    

    return Result;
}

internal uint32 GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position ChunkPos = GetChunkPosition(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ); 
    uint32 Value = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
    return Value;
}

internal uint32 GetTileValue(tile_map *TileMap, tile_map_postition *Pos)
{
    uint32 Value = GetTileValue(TileMap, Pos->AbsTileX, Pos->AbsTileY, Pos->AbsTileZ);
    return Value;
}

bool CanMove(tile_map *TileMap, tile_map_postition *Pos)
{
    uint32 Tile = GetTileValue(TileMap, Pos);
    
    return Tile != 2;
}

inline void RecanonicalazeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel)
{
    int32 Offset = RoundReal32toInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= TileMap->TileSideInMeters * Offset;
    
    Assert(*TileRel >= -0.5f*TileMap->TileSideInMeters);
    Assert(*TileRel <= 0.5f*TileMap->TileSideInMeters);
}

inline void RecanonicalizePostion(tile_map *World, tile_map_postition *Pos)
{
    RecanonicalazeCoord(World, 
        &Pos->AbsTileX, &Pos->RelTileX);
    RecanonicalazeCoord(World, 
        &Pos->AbsTileY, &Pos->RelTileY);
}

inline void SetTileValue(memory_arena *Arena, tile_map *TileMap, 
                        uint32 X, uint32 Y, uint32 Z,
                        uint32 Val)
{
    tile_chunk_position ChunkPos = GetChunkPosition(TileMap, X, Y, Z);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ); 
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

inline bool IsOnSameTile(tile_map_postition *Pos1, tile_map_postition *Pos2)
{
    bool Result = (Pos1->AbsTileX == Pos2->AbsTileX) &&
        (Pos1->AbsTileY == Pos2->AbsTileY) &&
        (Pos1->AbsTileZ == Pos2->AbsTileZ);
    return Result;
}