#include "handmade.h"

internal void OutputSound(game_sound_output_buffer *SoundBuffer)
{
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    int ToneHz = 256;
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
    game_offscreen_buffer *Buffer,
    int xOffset, int yOffset,
    game_sound_output_buffer *SoundBuffer)
{
    OutputSound(SoundBuffer);
    RenderGradient(Buffer, xOffset, yOffset);
}