
#include "handmade.h"

#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNNK 16

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

internal bool
IsCanonical(world *World, real32 TileRel)
{
    bool Result = TileRel >= -0.5f * World->ChunkSideInMeters &&
                  TileRel <=  0.5f * World->ChunkSideInMeters;
    return Result;
}

internal bool
IsCanonical(world *World, v2 Offset)
{
    bool Result = IsCanonical(World, Offset.X) && 
                  IsCanonical(World, Offset.Y);
    return Result;
}


inline void 
RecanonicalazeCoord(world *World, int32 *Tile, real32 *TileRel)
{
    int32 Offset = RoundReal32toInt32(*TileRel / World->ChunkSideInMeters);
    *Tile += Offset;
    *TileRel -= World->ChunkSideInMeters * Offset;
    Assert(IsCanonical(World, *TileRel));
}

// inline void RecanonicalizePostion(tile_map *World, tile_map_postition *Pos)
// {
//     RecanonicalazeCoord(World, 
//         &Pos->AbsTileX, &Pos->RelTile.X);
//     RecanonicalazeCoord(World, 
//         &Pos->AbsTileY, &Pos->RelTile.Y);
// }

inline world_postition
MapIntoChunkSpace(world *World, world_postition BasePos, v2 Offset)
{
    world_postition Result = BasePos;
    Result.RelTile += Offset;
    RecanonicalazeCoord(World, 
        &Result.ChunkX, &Result.RelTile.X);
    RecanonicalazeCoord(World, 
        &Result.ChunkY, &Result.RelTile.Y);
    return Result;
}

inline bool 
AreOnSameChunk(world *World, world_postition *Pos1, world_postition *Pos2)
{
    Assert(IsCanonical(World, Pos1->RelTile));
    Assert(IsCanonical(World, Pos2->RelTile));
    bool Result = (Pos1->ChunkX == Pos2->ChunkX) &&
                  (Pos1->ChunkY == Pos2->ChunkY) &&
                  (Pos1->ChunkZ == Pos2->ChunkZ);
    return Result;
}

inline v2 
Substract(world *World, world_postition *A, world_postition *B)
{
    v2 Diff = {};
    
    v2 DTile = V2(
        (real32)A->ChunkX - (real32)B->ChunkX, 
        (real32)A->ChunkY - (real32)B->ChunkY);

    Diff = World->ChunkSideInMeters * DTile  + (A->RelTile - B->RelTile);
    return Diff;
}

inline world_postition
ChunkPositionFromTilePOstition(world *World, 
    int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
    world_postition Res = {};
    Res.ChunkX = AbsTileX / TILES_PER_CHUNNK;
    Res.ChunkY = AbsTileY / TILES_PER_CHUNNK;
    Res.ChunkZ = AbsTileZ / TILES_PER_CHUNNK;
    Res.RelTile.X = (real32)(AbsTileX - (Res.ChunkX*TILES_PER_CHUNNK)) * World->TileSideInMeters;
    Res.RelTile.Y = (real32)(AbsTileY - (Res.ChunkY*TILES_PER_CHUNNK)) * World->TileSideInMeters;
    world_postition P = MapIntoChunkSpace(World, Res, V2(0,0));

    return P;
}

inline world_postition 
CenteredChunkPoint(int32 ChunkX, int32 ChunkY, int32 ChunkZ)
{
    world_postition Result = {};

    Result.ChunkX = ChunkX;
    Result.ChunkY = ChunkY;
    Result.ChunkZ = ChunkZ;

    return Result;
}

internal void
InitializeTileMap(world *World)
{
    World->TileSideInMeters = 1.4f;
    World->ChunkSideInMeters = (real32)TILES_PER_CHUNNK * World->TileSideInMeters;
    World->FirstFree = 0;
    for (uint32 WorldChunkIndex = 0; 
        WorldChunkIndex < ArrayCount(World->ChunkHash); 
        WorldChunkIndex++)
    {
        World->ChunkHash[WorldChunkIndex].X = TILE_CHUNK_UNINITIALIZED;
        World->ChunkHash[WorldChunkIndex].FirstBlock.EntitiesCount = 0;
    }
}

inline void
ChangeEntityLocation(memory_arena *Arena, world *World, uint32 ColdEntityIdx, 
                    world_postition *OldP, world_postition *NewP)
{
    if (OldP && AreOnSameChunk(World, OldP, NewP))
    {
        return;
    }
    if (OldP)
    {
        world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, Arena);
        Assert(Chunk);

        if (Chunk)
        {
            world_entity_block *FirstBlock = &Chunk->FirstBlock;
            bool Found = false;
            for (world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
            {
                if (Found)
                {
                    break;
                }
                for (uint32 EntityIdx = 0; EntityIdx < Block->EntitiesCount; EntityIdx++)
                {
                    if (Block->LowEntityIndex[EntityIdx] == ColdEntityIdx)
                    {
                        Block->LowEntityIndex[EntityIdx] = FirstBlock->LowEntityIndex[FirstBlock->EntitiesCount--];
                        if (FirstBlock->EntitiesCount == 0)
                        {
                            if (FirstBlock->Next)
                            {
                                world_entity_block *NextBlock  = FirstBlock->Next;
                                *FirstBlock = *NextBlock;
                                NextBlock->Next = World->FirstFree;
                                World->FirstFree = NextBlock;
                            }
                        }
                        Found = true;
                        break;
                    }
                }      
            }   
        }
    }
    world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
    world_entity_block *Block = &Chunk->FirstBlock;
    if (Block->EntitiesCount == ArrayCount(Block->LowEntityIndex))
    {
        world_entity_block *OldBlock = PushStruct(Arena, world_entity_block);
        *OldBlock = *Block;
        Block->Next = OldBlock;
        Block->EntitiesCount = 0;
    }
    Block->LowEntityIndex[Block->EntitiesCount++] = ColdEntityIdx;
}