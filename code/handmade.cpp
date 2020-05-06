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

internal void RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY)
{
    if (PlayerX < 0 || PlayerX + 10 > Buffer->Width)
    {
        return;
    }

    if (PlayerY < 0 || PlayerY + 10 > Buffer->Height)
    {
        return;
    }

    uint32 *Pixel = (uint32 *)Buffer->Memory + PlayerX + PlayerY * Buffer->Width;
    for (int y = 0; y < 10; y++)
    {
        for (int x = 0; x < 10; x++)
        {
            *Pixel++ = 0xFFFFFF;
        }
        Pixel += (Buffer->Width - 10);
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
    
    if (Input0.Up.EndedDown)
    {
        GameState->PlayerY -= 1;
    }

    if (Input0.Down.EndedDown)
    {
        GameState->PlayerY += 1;
    }

    if (Input0.Left.EndedDown)
    {
        GameState->PlayerX -= 1;
    }

    if (Input0.Right.EndedDown)
    {
        GameState->PlayerX += 1;
    }

    if (Input0.Space.EndedDown)
    {
        GameState->PlayerTimer = 4.0;
    }

    if (GameState->PlayerTimer > 0)
    {
        GameState->PlayerY += (int)(10.0f*sinf(win32_PI/2 * GameState->PlayerTimer));
        GameState->PlayerTimer -= (real32)0.066;
    }
    
    
    RenderGradient(Buffer, GameState->GreenOffset, GameState->BlueOffset);
    RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    local_persist int ToneVolume = 0;
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