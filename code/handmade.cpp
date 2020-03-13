#include "handmade.h"

internal void OutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz, int ToneVolume)
{
    local_persist real32 tSine;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for (DWORD SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {

        real32 sineValue = sinf(tSine);
        int16 SampleValue = (int16)(sineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * win32_PI * 1.0f / (real32)WavePeriod;
    }
}

internal void RenderGradient(
    game_offscreen_buffer *Buffer, 
    int xOffset, int yOffset)
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

internal void GameUpdateAndRender(
    game_memory *Memory,
    game_offscreen_buffer *Buffer,
    game_sound_output_buffer *SoundBuffer,
    game_input *Input)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->isInitialized)
    {

        char *Filename = "test.bmp";
        debug_file_read FileReadResult = DEBUGPlatformReadEntireFile(Filename);
        if(FileReadResult.ContentSize)
        {
            DEBUGPlatformWriteEntireFile(Filename, 300, "asd");
            DEBUGPlatformFreeFileMemory(FileReadResult.Content);
        }

        GameState->ToneHz = 256;

        Memory->isInitialized = true;
    }

    game_controller_input Input0 = Input->Controllers[0];
    if (Input0.IsAnalog)
    {
        
    }
    else
    {
        
    }
    
    RenderGradient(Buffer, GameState->GreenOffset, GameState->BlueOffset);

    local_persist int ToneVolume = 1000;
    OutputSound(SoundBuffer, GameState->ToneHz, ToneVolume);
}