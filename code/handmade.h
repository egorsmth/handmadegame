#if !defined(HANDMADE_H)

#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define win32_PI 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

#if SLOW_CODE
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)

struct thread_context
{
    int Placeholder;
};

#if INTERNAL_BUILD
struct debug_file_read
{
    void *Content;
    uint32 ContentSize;    
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_file_read name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(thread_context *Thread, char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif


inline uint32 SafeTruncateUint32(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_sound_output_buffer
{
    int16 *Samples;
    int SampleCount;
    int SamplesPerSecond;
};

struct game_button_state
{
    int HalfTransitionCount;
    bool EndedDown;
};

struct game_controller_input
{
    bool IsAnalog;

    union
    {
        game_button_state Buttons[8];
        struct {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
            game_button_state L;
            game_button_state Space;
        };
    };
};

struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX, MouseY, MouseZ;

    real64 SecondsToAdvance;
    
    game_controller_input Controllers[4];
};

struct game_memory
{
    bool isInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage;
    uint64 TransientStorageSize;
    void *TransientStorage;

    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_offscreen_buffer *Buffer, game_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub) {}

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer, game_input *Input)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub) {}

struct tile_map
{
    uint32 *Map;
};

struct world
{
    real32 TileSideInMeters;
    int32 TileSideInPixels;

    int32 CountX;
    int32 CountY;

    int32 TileMapCountX;
    int32 TileMapCountY;
    tile_map *TileMaps;
};

struct world_coordibate
{
    int32 WorldTileX;
    int32 WorldTileY;

    int32 TileX;
    int32 TileY;

    real32 X;
    real32 Y;
};

struct game_state
{
    int32 PlayerWorldX;
    int32 PlayerWorldY;
    
    real32 PlayerX;
    real32 PlayerY;
};

#define HANDMADE_H
#endif