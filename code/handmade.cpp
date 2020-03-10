#include "handmade.h"

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
    int xOffset, int yOffset)
{
    RenderGradient(Buffer, xOffset, yOffset);
}