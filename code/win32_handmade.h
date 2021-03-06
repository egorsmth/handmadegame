#if !defined(H_WIN32_HANDMADE)

struct win32_offscreen_buffer 
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dims
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    int SafetyBytes;
};

struct win32_debug_time_marker
{
    DWORD PlayCursor;
    DWORD WriteCursor;
};

struct win32_game_code
{
    HMODULE GameCodeDLL;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;
    FILETIME LastFileTime;
    bool isValid;
};

struct win32_game_loop
{
    HANDLE Journal;
    HANDLE Tempjournal;
    uint64 JournalSize;
    void *GameMemoryBlock;

    int IndexSave;
    int IndexPlay;

    char MainEXEFileName[MAX_PATH];
    DWORD SizeOfFile;
    char *OnePastLastSlashFilename;
};

#define H_WIN32_HANDMADE
#endif