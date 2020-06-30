#if !defined(WORLD_H)

struct world_entity_block
{
    uint32 EntitiesCount;
    uint32 LowEntityIndex[16];
    world_entity_block *Next;
};

struct world_chunk
{
    uint32 X;
    uint32 Y;
    uint32 Z;

    world_entity_block FirstBlock;

    world_chunk *NextInHash;
};

struct world
{
    real32 TileSideInMeters;
    real32 ChunkSideInMeters;

    world_entity_block *FirstFree;

    world_chunk ChunkHash[4096];
};

struct world_postition
{
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;

    v2 RelTile;
};

struct world_map_diff
{
    v2 D;
    // Dz is not used yet since z coordinate not fully implemented
    real32 Dz;
};

#define WORLD_H
#endif