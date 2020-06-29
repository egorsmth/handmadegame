#if !defined(HANDMADE_H)

#include <math.h>
#include <stdint.h>

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM

#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif

#endif

#if COMPILER_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

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

typedef size_t memory_index;

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
#define INTERNAL_BUILD 1
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

inline int32 RoundReal32toInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
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
#include "handmade_math.h"
#include "world.h"

#define Minimum(A, B) ((A > B) ? (B) : (A))
#define Maximum(A, B) ((A < B) ? (B) : (A))

struct memory_arena
{
    memory_index Size;
    uint8 * Base;
    memory_index Used;
};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32 *Pixels;
};

struct hero_bitmaps
{
    loaded_bitmap Head;
    loaded_bitmap Body;
    loaded_bitmap Legs;
};

enum entity_type
{
    EntityType_Player,
    EntityType_Wall
};

struct hot_entity
{
    bool Exist;
    v2 P; // relative to the camera
    v2 dP;
    uint32 AbsTileZ;
    uint32 FacingDirection;

    uint32 ColdIndex;
};

struct cold_entity
{
    entity_type Type;
    world_postition P;
    real32 Width, Height;
    int32 dAbsTileZ;
    bool Collides;

    uint32 HotIndex;
};

struct entity
{
    hot_entity *Hot;
    cold_entity *Cold;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    uint32 PlayerEntityIndex;

    uint32 HotEntityCount;
    hot_entity HotEntity[256];

    uint32 ColdEntityCount;
    cold_entity ColdEntity[5256];

    world_postition CameraP;
    loaded_bitmap Backdrop;
    hero_bitmaps HeroBitmap[4];
};

#define PushStruct(Arena, type) (type *)PushStruct_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushStruct_(Arena, (Count) * sizeof(type))

void *PushStruct_(memory_arena *Arena, memory_index Size)
{
    Assert(Arena->Used + Size <= Arena->Size);
    void * Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}

#define HANDMADE_H
#endif