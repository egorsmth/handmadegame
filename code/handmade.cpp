#include "handmade.h"

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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {

        char *Filename = "test.bmp";
        debug_file_read FileReadResult = Memory->DEBUGPlatformReadEntireFile(Filename);
        if(FileReadResult.ContentSize)
        {
            Memory->DEBUGPlatformWriteEntireFile(Filename, 300, "asd");
            Memory->DEBUGPlatformFreeFileMemory(FileReadResult.Content);
        }      

        Memory->isInitialized = true;
    }

    game_controller_input Input0 = Input->Controllers[0];
    if (Input0.Up.EndedDown)
    {
        GameState->BlueOffset += 1;
    }
    else
    {
        
    }
    
    RenderGradient(Buffer, GameState->GreenOffset, GameState->BlueOffset);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    local_persist int ToneVolume = 1000;
    if (GameState->ToneHz == 0) 
    {
        GameState->ToneHz = 256;
        GameState->tSine = 0.0f;
    }
    game_controller_input Input0 = Input->Controllers[0];
    if (Input0.Up.EndedDown)
    {
        
        GameState->ToneHz += 1;
    }
    OutputSound(SoundBuffer, GameState, ToneVolume);
}