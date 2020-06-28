#if !defined(TILE_H)

struct tile_chunk
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;

    uint32 *Map;

    tile_chunk *NextInHash;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;
    
    real32 TileSideInMeters;
    int32 TileSideInPixels;
    int32 PixPerMeter;

    tile_chunk TileChunkHash[4096];

};

struct tile_map_postition
{
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;

    v2 RelTile;
};

struct tile_map_diff
{
    v2 D;
};

#define TILE_H
#endif