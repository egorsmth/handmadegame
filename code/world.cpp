
#include "handmade.h"

#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline world_chunk*
GetWorldChunk(world *World, uint32 X, uint32 Y, uint32 Z,
            memory_arena *Arena = 0)
{
    uint32 HashIndex = 19*X + 7*Y + 3*Z;
    uint32 HashSlot = HashIndex & (ArrayCount(World->ChunkHash) - 1);
    world_chunk *Chunk = &World->ChunkHash[HashSlot];
    do
    {
        if (
            X == Chunk->X &&
            Y == Chunk->Y &&
            Z == Chunk->Z)
        {
            break;
        }
        if (Arena && (Chunk->X != TILE_CHUNK_UNINITIALIZED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, world_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->X = TILE_CHUNK_UNINITIALIZED;
        }

        if (Chunk->X == TILE_CHUNK_UNINITIALIZED && Arena)
        {
            Chunk->X = X;
            Chunk->Y = Y;
            Chunk->Z = Z;
            Chunk->NextInHash = 0;
            break;
        }

        Chunk = Chunk->NextInHash;
    } while (Chunk);
    
    return Chunk;
}

internal bool 
IsTileValueEmpty(uint32 Tile)
{
    return Tile != 2;
}

inline void 
RecanonicalazeCoord(world *World, uint32 *Tile, real32 *TileRel)
{
    int32 Offset = RoundReal32toInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= World->TileSideInMeters * Offset;
    
    Assert(*TileRel >= -0.5f*World->TileSideInMeters);
    Assert(*TileRel <= 0.5f*World->TileSideInMeters);
}

// inline void RecanonicalizePostion(tile_map *World, tile_map_postition *Pos)
// {
//     RecanonicalazeCoord(World, 
//         &Pos->AbsTileX, &Pos->RelTile.X);
//     RecanonicalazeCoord(World, 
//         &Pos->AbsTileY, &Pos->RelTile.Y);
// }

inline world_postition
MapIntoTileSpace(world *World, world_postition BasePos, v2 Offset)
{
    world_postition Result = BasePos;
    Result.RelTile += Offset;
    RecanonicalazeCoord(World, 
        &Result.AbsTileX, &Result.RelTile.X);
    RecanonicalazeCoord(World, 
        &Result.AbsTileY, &Result.RelTile.Y);
    return Result;
}

inline bool 
IsOnSameTile(world_postition *Pos1, world_postition *Pos2)
{
    bool Result = (Pos1->AbsTileX == Pos2->AbsTileX) &&
        (Pos1->AbsTileY == Pos2->AbsTileY) &&
        (Pos1->AbsTileZ == Pos2->AbsTileZ);
    return Result;
}

inline v2 
Substract(world *World, world_postition *A, world_postition *B)
{
    v2 Diff = {};
    
    v2 DTile = V2((real32)A->AbsTileX - (real32)B->AbsTileX, (real32)A->AbsTileY - (real32)B->AbsTileY);

    Diff = World->TileSideInMeters * DTile  + (A->RelTile - B->RelTile);
    return Diff;
}

inline world_postition 
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_postition Result = {};

    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;

    return Result;
}

internal void
InitializeTileMap(world *World)
{
    World->ChunkShift = 8;
    World->ChunkMask = (1 << World->ChunkShift) - 1;
    World->ChunkDim = (1 << World->ChunkShift);

    World->TileSideInMeters = 1.4f;
    World->TileSideInPixels = 60;
    World->PixPerMeter = RoundReal32toInt32((real32)World->TileSideInPixels / World->TileSideInMeters);

    for (uint32 WorldChunkIndex = 0; 
        WorldChunkIndex < ArrayCount(World->ChunkHash); 
        WorldChunkIndex++)
    {
        World->ChunkHash[WorldChunkIndex].X = TILE_CHUNK_UNINITIALIZED;
    }
}