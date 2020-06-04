#include "handmade.h"

#include <windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_handmade.h"

global_variable bool RUNNING;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 PerfCountFrequency;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {return ERROR_DEVICE_NOT_CONNECTED;}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVIbration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {return ERROR_DEVICE_NOT_CONNECTED;}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void ConcatStrings(
    size_t SizeA, char *A,
    size_t SizeB, char *B,
    char *Dest
)
{
    for (size_t i = 0; i < SizeA; i++)
    {
        *Dest++ = *A++;
    }
    
    for (size_t i = 0; i < SizeB; i++)
    {
        *Dest++ = *B++;
    }
    *Dest++ = 0;
}

internal void Win32GetOnePastLastSlash(win32_game_loop *GameState)
{
    GameState->OnePastLastSlashFilename = GameState->MainEXEFileName;
    for (char *Scan = GameState->MainEXEFileName; *Scan; ++Scan)
    {
        if (*Scan == '\\')
        {
            GameState->OnePastLastSlashFilename = Scan + 1;
        }
    }
}

internal int StringLength(char *S)
{
    int Count = 0;
    while(*S++)
    {
        Count++;
    }
    return Count;
}

internal void Win32GetFullPath(win32_game_loop *GameState, char *Filename, char* Dest)
{
    ConcatStrings(
        GameState->OnePastLastSlashFilename - GameState->MainEXEFileName,
        GameState->MainEXEFileName, 
        StringLength(Filename), Filename,
        Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_file_read Result = {};
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUint32(FileSize.QuadPart);
            Result.Content = VirtualAlloc(0, FileSize.QuadPart, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            if (Result.Content)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Content, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
                {
                    Result.ContentSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Thread, Result.Content);
                    Result.Content = 0;
                }
            }
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool Result = false;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWriten;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWriten, 0) && (BytesWriten == MemorySize))
        {
            Result = true;               
        }
        else
        {

        }
        CloseHandle(FileHandle);
        
    }
    return Result;
}

inline FILETIME Win32GetLastWriteTime(char *Filename)
{
    FILETIME ft = {};
    WIN32_FILE_ATTRIBUTE_DATA ad = {};
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &ad))
    {
        ft = ad.ftLastWriteTime;
    }
    return ft;
}

internal win32_game_code Win32LoadGameCode(char *SourceDLL, char* TempDLL)
{
    win32_game_code Result = {};   
    CopyFile(SourceDLL, TempDLL, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLL);
    if (Result.GameCodeDLL)
    {
        Result.GetSoundSamples = (game_get_sound_samples *)
            GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
        
        Result.UpdateAndRender = (game_update_and_render *)
            GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

        Result.isValid = (Result.GetSoundSamples && Result.UpdateAndRender);
        FILETIME ft = Win32GetLastWriteTime(SourceDLL);
        Result.LastFileTime = ft;
    }

    if (!Result.isValid)
    {
        Result.GetSoundSamples = GameGetSoundSamplesStub;
        Result.UpdateAndRender = GameUpdateAndRenderStub;
    }

    return Result;
}

internal void Win32UnloadGameCode(win32_game_code *GameCode)
{
    if (GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
    }

    GameCode->isValid = false;
    GameCode->GetSoundSamples = GameGetSoundSamplesStub;
    GameCode->UpdateAndRender = GameUpdateAndRenderStub;
}

internal void Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState) {XInputSetState = XInputSetStateStub;}
    }
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                DSBUFFERDESC BufferDesc = {};
                BufferDesc.dwSize = sizeof(BufferDesc);
                BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("Primary sound buffer initialized\n");
                    }
                    else
                    {
                        
                    }
                    
                }
            }
            else
            {
                
            }

            DSBUFFERDESC BufferDesc = {};
            BufferDesc.dwSize = sizeof(BufferDesc);
            BufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDesc.dwBufferBytes = BufferSize;
            BufferDesc.lpwfxFormat = &WaveFormat;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalSecondaryBuffer, 0)))
            {                
                OutputDebugStringA("Secondary sound buffer initialized\n");
            }
            
        }
        else 
        {

        }
    }
    else
    {
        
    }
    
}

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
        0,
        SoundOutput->SecondaryBufferSize,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        int8 *DestSample = (int8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        DestSample = (int8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(
            Region1, Region1Size,
            Region2, Region2Size);
    }
}

internal void Win32FillSoundBuffer(
    win32_sound_output *SoundOutput,
    DWORD ByteToLock,
    DWORD BytesToWrite,
    game_sound_output_buffer *SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
        ByteToLock,
        BytesToWrite,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16 *SourceSample = SourceBuffer->Samples;
        int16 *DestSample = (int16 *)Region1;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DestSample = (int16 *)Region2;
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(
            Region1, Region1Size,
            Region2, Region2Size);
    }
}

internal void Win32ProcessKeyboardEvent(game_button_state *NewState, bool IsDown)
{
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

internal void Win32ResizeDIBSection(
    win32_offscreen_buffer *Buffer, 
    int width, int height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    Buffer->Width = width;
    Buffer->Height = height;
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = width;
    Buffer->Info.bmiHeader.biHeight = -height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    Buffer->BytesPerPixel = 4;
    Buffer->Pitch = width * Buffer->BytesPerPixel;

    int BitmapMemorySize = Buffer->BytesPerPixel * width * height;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferWindow(
    win32_offscreen_buffer *Buffer,
    HDC DeviceContext,
    int Width, int Height) 
{
    if (Width > Buffer->Width * 2) // also check hight
    {
        StretchDIBits(
            DeviceContext,
            0, 0, Width, Height,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Memory,
            &Buffer->Info,
            DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        PatBlt(DeviceContext, 0, 0, Width, Height, BLACKNESS);
        StretchDIBits(
            DeviceContext,
            0, 0, Buffer->Width, Buffer->Height,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Memory,
            &Buffer->Info,
            DIB_RGB_COLORS, SRCCOPY);
    }
}

internal win32_window_dims Win32GetWindowDims(HWND WindowHandler)
{
    HDC DeviceContext = GetDC(WindowHandler);
    RECT WindowRect;
    GetClientRect(WindowHandler, &WindowRect);
    win32_window_dims Result;
    Result.Width = WindowRect.right - WindowRect.left;
    Result.Height = WindowRect.bottom- WindowRect.top;
    return Result;
}

global_variable WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };
internal void Win32TogleFullScreen(HWND hwnd)
{
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (dwStyle & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hwnd, &g_wpPrev) &&
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(hwnd, GWL_STYLE,dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else 
    {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_wpPrev);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

LRESULT CALLBACK Win32MainWindowCallback(
  HWND   hwnd,
  UINT   uMsg,
  WPARAM wParam,
  LPARAM lParam
) {
    LRESULT Result = 0;
    switch (uMsg)
    {
        case WM_SIZE:
        {            
            OutputDebugStringA("WM_SIZE\n");
        }
            break;
        case WM_DESTROY:
        {
            RUNNING = false;        
            OutputDebugStringA("WM_DESTROY\n");
        }
            break;
        case WM_CLOSE:
        {
            RUNNING = false;
            OutputDebugStringA("WM_CLOSE\n");
        }
            break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVEAPP\n");
        }
            break;
        case WM_PAINT:
        {
            OutputDebugStringA("WM_PAINT\n");
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(hwnd, &Paint);
            int x = Paint.rcPaint.left;
            int y = Paint.rcPaint.top;
            int w = Paint.rcPaint.right - Paint.rcPaint.left;
            int h = Paint.rcPaint.bottom - Paint.rcPaint.top;

            RECT ClientRectangle;
            GetClientRect(hwnd, &ClientRectangle);
            win32_window_dims Dimensions = Win32GetWindowDims(hwnd);
            Win32DisplayBufferWindow(
                &GlobalBackBuffer,
                DeviceContext,
                Dimensions.Width, Dimensions.Height);
            EndPaint(hwnd, &Paint);
        }
            break;
        default:
        {
            Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
            break;
    }
    return Result;
}

internal void GetInputFileLocation(win32_game_loop *GameState, char *Dest)
{
    Win32GetFullPath(GameState, "loop_edit.hmi", Dest);
}

internal void Win32BeginRecordingInput(win32_game_loop *GameState, int InputRecordingIndex)
{
    GameState->IndexSave = InputRecordingIndex;

    char FullFilename[MAX_PATH];
    GetInputFileLocation(GameState, FullFilename);
    GameState->Journal = CreateFileA(FullFilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    
    DWORD BytesToWrite = (DWORD)GameState->JournalSize;
    Assert(BytesToWrite == GameState->JournalSize);
    DWORD BytesWriten;
    WriteFile(GameState->Journal, GameState->GameMemoryBlock, BytesToWrite, &BytesWriten, 0);

}

internal void win32EndRecordingInput(win32_game_loop *GameState)
{
    CloseHandle(GameState->Journal);
    GameState->IndexSave = 0;
}

internal void Win32BeginInputPlayback(win32_game_loop *GameState, int InputPlayIndex)
{
    GameState->IndexPlay = InputPlayIndex;

    char FullFilename[MAX_PATH];
    GetInputFileLocation(GameState, FullFilename);
    GameState->Tempjournal = CreateFileA(FullFilename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD BytesToRead = (DWORD)GameState->JournalSize;
    Assert(GameState->JournalSize == BytesToRead);
    DWORD BytesRead;
    ReadFile(GameState->Tempjournal, GameState->GameMemoryBlock, BytesToRead, &BytesRead, 0);
}

internal void Win32EndInputPlayback(win32_game_loop *GameState)
{
    CloseHandle(GameState->Tempjournal);
    GameState->IndexPlay = 0;
}

internal void Win32RecordInput(win32_game_loop *GameState, game_input *Input)
{
    DWORD BytesWriten;
    WriteFile(GameState->Journal, Input, sizeof(*Input), &BytesWriten, 0);
}

internal void Win32PlaybackInput(win32_game_loop *GameState, game_input *NewInput)
{
    DWORD BytesRead;
    ReadFile(GameState->Tempjournal, NewInput, sizeof(*NewInput), &BytesRead, 0);
    if (BytesRead)
    {

    }
    else
    {
        int PlayIndex = GameState->IndexPlay;
        Win32EndInputPlayback(GameState);
        Win32BeginInputPlayback(GameState, PlayIndex);
    }
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController, win32_game_loop *GameState)
{
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        if (Message.message == WM_QUIT)
        {
            RUNNING = false;
        }
        
        switch (Message.message)
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKcode = (uint32)Message.wParam;
                bool wasDown = ((Message.lParam & (1 << 30)) != 0);
                bool isDown = ((Message.lParam & (1 << 31)) == 0);
                
                if (isDown == wasDown) break;
                
                if (VKcode == 'W')
                {
                    OutputDebugStringA("KEY W\n");
                    Win32ProcessKeyboardEvent(&KeyboardController->Up, isDown);
                }
                else if (VKcode == 'A')
                {}
                else if (VKcode == 'S')
                {}
                else if (VKcode == 'D')
                {}
                else if (VKcode == 'E')
                {
                    Win32ProcessKeyboardEvent(&KeyboardController->RightShoulder, isDown);
                }
                else if (VKcode == 'Q')
                {
                    Win32ProcessKeyboardEvent(&KeyboardController->LeftShoulder, isDown);
                }
                else if (VKcode == 'L')
                {
                    if (isDown)
                    {
                        if (GameState->IndexSave == 0)
                        {
                            Win32BeginRecordingInput(GameState, 1);
                        }
                        else 
                        {
                            win32EndRecordingInput(GameState);
                            Win32BeginInputPlayback(GameState, 1);
                        }
                    }
                }
                else if (VKcode == VK_SPACE)
                {
                    OutputDebugStringA("KEY SPACE\n");
                    // Win32ProcessKeyboardEvent(&KeyboardController->Space, isDown);
                    
                    if (Message.hwnd && isDown)
                    {
                        Win32TogleFullScreen(Message.hwnd);
                    }
                }
                else if (VKcode == VK_UP)
                {
                    OutputDebugStringA("KEY UP\n");
                    Win32ProcessKeyboardEvent(&KeyboardController->Up, isDown);
                }
                else if (VKcode == VK_LEFT)
                {
                    Win32ProcessKeyboardEvent(&KeyboardController->Left, isDown);
                }
                else if (VKcode == VK_DOWN)
                {
                    Win32ProcessKeyboardEvent(&KeyboardController->Down, isDown);
                }
                else if (VKcode == VK_RIGHT)
                {
                    Win32ProcessKeyboardEvent(&KeyboardController->Right, isDown);
                }
                else if (VKcode == VK_ESCAPE)
                {
                    RUNNING = false;
                }
            } break;
        
        default:
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
            break;
        }
    }
}

inline LARGE_INTEGER Win32GetWallClock(void)
{
    LARGE_INTEGER EndCounter;
    QueryPerformanceCounter(&EndCounter);
    return EndCounter;
}

inline real64 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real64 Result = ((real64)End.QuadPart - (real64)Start.QuadPart) /
                                    (real64)PerfCountFrequency;
    return Result;
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer *GlobalBackBuffer,
                                    int x, 
                                    int Top, 
                                    int Bottom,
                                    int Color)
{
    uint8 *Pixel = ((uint8 *)GlobalBackBuffer->Memory + 
                    x*GlobalBackBuffer->BytesPerPixel +
                    Top*GlobalBackBuffer->Pitch);
    for (int Y = Top; Y < Bottom; Y++)
    {
        *(uint32 *)Pixel = Color; 
        Pixel += GlobalBackBuffer->Pitch;
    }
}

internal void DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer,
                                    win32_sound_output *SoundOut,
                                    real64 C, int PadX, int Top, int Bottom,
                                    DWORD Value, uint32 Color)
{
    real64 XReal64 = (C * (real64)Value);
    int x = PadX + (int)XReal64;
    Win32DebugDrawVertical(BackBuffer, x, Top, Bottom, Color);
}

internal void Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer,
                                    int MarkerCount,
                                    win32_debug_time_marker *Markers,
                                    int CurrentMarkerIndex,
                                    win32_sound_output *SoundOut, 
                                    real64 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;
    int LineHeight = 64;
    

    real32 C = ((real32)BackBuffer->Width - 2*PadX) / (real32)SoundOut->SecondaryBufferSize;
    for (int PlayCursorIndex = 0; PlayCursorIndex < MarkerCount; PlayCursorIndex++)
    {
        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;

        int Top = PadY;
        int Bottom = PadY + LineHeight;
        if (PlayCursorIndex == CurrentMarkerIndex)
        {
            Top += PadY + LineHeight;
            Bottom += PadY + LineHeight;
        }

        win32_debug_time_marker *ThisMarker = &Markers[PlayCursorIndex];
        DrawSoundBufferMarker(BackBuffer, SoundOut, C, PadX, Top, Bottom, ThisMarker->PlayCursor, PlayColor);
        DrawSoundBufferMarker(BackBuffer, SoundOut, C, PadX, Top, Bottom, ThisMarker->WriteCursor, WriteColor);
    }
}

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
{
    win32_game_loop GameState = {};
    DWORD SizeOfFile = GetModuleFileNameA(0, GameState.MainEXEFileName, sizeof(GameState.MainEXEFileName));
    GameState.SizeOfFile = SizeOfFile;
    Win32GetOnePastLastSlash(&GameState);

    char SourceGameCodeFilename[] = "handmade.dll";
    char SourceGameCodeFullPath[MAX_PATH];
    Win32GetFullPath(&GameState, SourceGameCodeFilename, SourceGameCodeFullPath);

    char TempGameCodeFilename[] = "temp_handmade.dll";
    char TempGameCodeFullPath[MAX_PATH];
    Win32GetFullPath(&GameState, TempGameCodeFilename, TempGameCodeFullPath);

    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);
    PerfCountFrequency = PerfFreq.QuadPart;

    UINT DesiredSchedulerMs = 1;
    bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMs) == TIMERR_NOERROR);

    Win32LoadXInput();
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, WINDOW_WIDTH, WINDOW_HEIGHT);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    bool SoundValid = false;

    if (RegisterClass(&WindowClass))
    {
        HWND WIndowHandle = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            WINDOW_WIDTH+40,
            WINDOW_HEIGHT+40,
            0,
            0,
            hInstance,
            0
        );
        if (WIndowHandle)
        {
            HDC DeviceContext = GetDC(WIndowHandle);
            int MonitorRefreshHz = GetDeviceCaps(DeviceContext, VREFRESH);
            if (MonitorRefreshHz <= 1)
            {
                MonitorRefreshHz = 60;
            }
            const real64 GameUpdateHz = MonitorRefreshHz / 2.0f;
            real64 TargetSecondsPerFrame = 1.0f / ((real64)GameUpdateHz);
            ReleaseDC(WIndowHandle, DeviceContext);

            win32_sound_output SoundOut = {};
            SoundOut.SamplesPerSecond = 48000;
            SoundOut.BytesPerSample = sizeof(int16)*2;
            SoundOut.SecondaryBufferSize = SoundOut.SamplesPerSecond*SoundOut.BytesPerSample;
            SoundOut.SafetyBytes = (int)(((SoundOut.SamplesPerSecond*SoundOut.BytesPerSample) 
                                    / GameUpdateHz) / 3.0f);

            Win32InitDSound(
                WIndowHandle, 
                SoundOut.SamplesPerSecond,
                SoundOut.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOut);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            RUNNING = true;

            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOut.SecondaryBufferSize,
                                                MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

// #if INTERNAL_BUILD
//             LPVOID BaseAddress = 0;
// #else
//             LPVOID BaseAddress = (LPVOID)(Gigabytes((uint64)2) * 1024);
// #endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes((uint64)1);
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameState.JournalSize = TotalSize;
            GameState.GameMemoryBlock = VirtualAlloc(0, TotalSize,
                                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.PermanentStorage = GameState.GameMemoryBlock;
            GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
            
            if (!Samples || !GameMemory.TransientStorage || !GameMemory.PermanentStorage)
            {
                OutputDebugStringA("Memory allocation problem\n");
                return(1);
            }
            else
            {
                OutputDebugStringA("Game Memory allocated\n");
            }
            
            LARGE_INTEGER LastCounter = Win32GetWallClock();

            int DebugTimeMarkersIndex = 0;
            win32_debug_time_marker DebugTimeMarkers[30] = {0};
        
            DWORD LastPlayCursor = 0;
            DWORD LastWriteCursor = 0;
            DWORD AudioLatencyBytes = 0;
            real32 AudioLatencySeconds = 0;

            char *SourceFilename = "handmade.dll"; 

            int64 LastCycleCount = __rdtsc();
            game_input Inp = {};
            win32_game_code GameCode = Win32LoadGameCode(SourceGameCodeFullPath, TempGameCodeFullPath);
            while(RUNNING) 
            {
                FILETIME newft = Win32GetLastWriteTime(SourceGameCodeFullPath);
                if (CompareFileTime(&newft, &GameCode.LastFileTime) != 0)
                {
                    Win32UnloadGameCode(&GameCode);
                    GameCode = Win32LoadGameCode(SourceGameCodeFullPath, TempGameCodeFullPath);
                }

                POINT MouseP;
                GetCursorPos(&MouseP);
                ScreenToClient(WIndowHandle, &MouseP);
                Inp.MouseX = MouseP.x;
                Inp.MouseY = MouseP.y;
                // Inp.MouseButtons[0] = 100;
                // Inp.MouseButtons[1] = 100;
                // Inp.MouseButtons[2] = 100;
                
                

                game_controller_input *KeyboardController = &Inp.Controllers[0];
                game_controller_input Zero = {};
                for (int i = 0; i < ArrayCount(KeyboardController->Buttons); i++)
                {
                    Zero.Buttons[i].EndedDown = KeyboardController->Buttons[i].EndedDown;
                }
                *KeyboardController = Zero;
                Win32ProcessPendingMessages(KeyboardController, &GameState);
                /*note: XUSER_MAX_COUNT-1 keyboard*/
                for (DWORD ControllerIndex = 0; ControllerIndex < 3; ControllerIndex++)
                {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;
                    }
                    else
                    {

                    }
                }
                game_offscreen_buffer OffscreenBuffer = {};
                OffscreenBuffer.Memory = GlobalBackBuffer.Memory;
                OffscreenBuffer.Width = GlobalBackBuffer.Width;
                OffscreenBuffer.Height = GlobalBackBuffer.Height;
                OffscreenBuffer.Pitch = GlobalBackBuffer.Pitch;
                OffscreenBuffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
                

                if (GameState.IndexSave)
                {
                    Win32RecordInput(&GameState, &Inp);
                }

                if (GameState.IndexPlay)
                {
                    Win32PlaybackInput(&GameState, &Inp);
                }
                thread_context Thread = {};
                Inp.SecondsToAdvance = TargetSecondsPerFrame;
                GameCode.UpdateAndRender(&Thread, &GameMemory, &OffscreenBuffer, &Inp);

                DWORD PlayCursor;
                DWORD WriteCursor;
                if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                {
                    if (!SoundValid)
                    {
                        SoundOut.RunningSampleIndex = WriteCursor / SoundOut.BytesPerSample;
                        SoundValid = true;
                    }

                    DWORD ByteToLock = (SoundOut.RunningSampleIndex*SoundOut.BytesPerSample) %
                                SoundOut.SecondaryBufferSize;

                    DWORD ExpectedBytesPerFrame = 
                                    (int)((real32)(SoundOut.SamplesPerSecond * SoundOut.BytesPerSample)
                                    / GameUpdateHz);
                    
                    DWORD ExpectedFrameBoudary = PlayCursor + ExpectedBytesPerFrame;
                    DWORD SafeWriteCursoe = WriteCursor;
                    if (SafeWriteCursoe < PlayCursor)
                    {
                        SafeWriteCursoe += SoundOut.SecondaryBufferSize;
                    }
                    SafeWriteCursoe += SoundOut.SafetyBytes;
                    bool AudioCardIsLatent = SafeWriteCursoe < ExpectedFrameBoudary;
                    
                    DWORD TargetCursor = 0;
                    if (AudioCardIsLatent)
                    {
                        TargetCursor = 
                            (WriteCursor + ExpectedFrameBoudary + ExpectedBytesPerFrame);
                    }
                    else
                    {
                        TargetCursor = 
                            (ExpectedFrameBoudary + ExpectedBytesPerFrame + SoundOut.SafetyBytes);
                    }
                    TargetCursor = TargetCursor % SoundOut.SecondaryBufferSize;
                    DWORD BytesToWrite = 0;
                    if (ByteToLock > TargetCursor) 
                    {
                        BytesToWrite = (SoundOut.SecondaryBufferSize - ByteToLock) + TargetCursor;
                    }
                    else
                    {
                        BytesToWrite = TargetCursor - ByteToLock;
                    }

                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOut.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOut.BytesPerSample;
                    SoundBuffer.Samples = Samples;
                    if (GameCode.GetSoundSamples)
                    {
                        GameCode.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer, &Inp);
                    }

#if INTERNAL_BUILD
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                    AudioLatencyBytes = WriteCursor - PlayCursor;
                    if (WriteCursor < PlayCursor)
                    {
                        AudioLatencyBytes += SoundOut.SecondaryBufferSize;
                    }
                    AudioLatencySeconds = ((real32)AudioLatencyBytes / (real32)SoundOut.BytesPerSample) /
                                                (real32)SoundOut.SamplesPerSecond;
                    
                    char TextBuffer[256];
                    _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                                "LPC:%u BTL:%u TC:%u  BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
                                LastPlayCursor, ByteToLock, TargetCursor, BytesToWrite,
                                PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                    OutputDebugString(TextBuffer);
#endif
                    Win32FillSoundBuffer(
                        &SoundOut, 
                        ByteToLock, BytesToWrite,
                        &SoundBuffer);
                } else {
                    SoundValid = false;
                }

                LARGE_INTEGER WorkCounter = Win32GetWallClock();
                real64 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                
                real64 SecondsElapsedPerFrame = WorkSecondsElapsed; 
                if (SecondsElapsedPerFrame < TargetSecondsPerFrame)
                {
                    if (SleepIsGranular)
                    {
                        DWORD SleepMs = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedPerFrame));
                        if (SleepMs > 0)
                        {
                            Sleep(SleepMs);
                        }
                    }

                    while (SecondsElapsedPerFrame < TargetSecondsPerFrame)
                    {                       
                        LARGE_INTEGER Cur = Win32GetWallClock();
                        SecondsElapsedPerFrame = Win32GetSecondsElapsed(LastCounter, Cur);
                    }
                }
                else
                {

                }
                LARGE_INTEGER EndCounter = Win32GetWallClock();
                real64 MsPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                LastCounter = EndCounter;

                win32_window_dims Dimensions = Win32GetWindowDims(WIndowHandle);

                Win32DisplayBufferWindow(
                    &GlobalBackBuffer,
                    DeviceContext,
                    Dimensions.Width, Dimensions.Height);
                // ReleaseDC(WIndowHandle, DeviceContext);
                
 //profiling
 #if 1
                uint64 EndCycleCount = __rdtsc();
                uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                LastCycleCount = EndCycleCount;

                real64 FPS = 0.0f;
                real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));
                
                char FPSBuffer[256];
                _snprintf_s(FPSBuffer, sizeof(FPSBuffer), "MS/frame: %.02f ms - FPS: %.02f - MegaCycles: %.02f\n", MsPerFrame, FPS, MCPF);
                OutputDebugStringA(FPSBuffer);
#endif
// profiling end
            }
        }
        else
        {}
    }
    else
    {}

    return 0;
}