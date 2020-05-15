#if !defined(TILE_H)

struct tile_chunk
{
    uint32 *Map;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_map
{
    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    tile_chunk *TileChunks;

    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;
    int32 TileSideInPixels;
    int32 PixPerMeter;
};

struct tile_map_postition
{
    uint32 AbsTileX;
    uint32 AbsTileY;

    real32 RelTileX;
    real32 RelTileY;
};


#define TILE_H
#endif