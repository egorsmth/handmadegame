#if !defined(HANDMADE_H)

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

internal void GameUpdateAndRender(
    game_offscreen_buffer *Buffer,
    int xOffset, int yOffset);

#define HANDMADE_H
#endif