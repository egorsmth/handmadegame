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
                    DEBUGPlatformFreeFileMemory(Result.Content);
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

struct win32_game_code
{
    HMODULE GameCodeDLL;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    bool isValid;
};

internal win32_game_code Win32LoadGameCode(void)
{
    win32_game_code Result = {};

    Result.GameCodeDLL = LoadLibraryA("handmade.dll");
    if (Result.GameCodeDLL)
    {
        Result.GetSoundSamples = (game_get_sound_samples *)
            GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
        
        Result.UpdateAndRender = (game_update_and_render *)
            GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

        Result.isValid = (Result.GetSoundSamples && Result.UpdateAndRender);
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
    StretchDIBits(
        DeviceContext,
        0, 0, Width, Height,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
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
internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController)
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
                else if (VKcode == VK_SPACE)
                {
                    
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

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
{
    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);
    PerfCountFrequency = PerfFreq.QuadPart;

    UINT DesiredSchedulerMs = 1;
    bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMs) == TIMERR_NOERROR);

    Win32LoadXInput();
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    const int MonitorRefreshHz = 60;
    const int GameUpdateHz = MonitorRefreshHz / 2;
    real64 TargetSecondsPerFrame = 1.0f / ((real64)GameUpdateHz);
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
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0
        );
        if (WIndowHandle)
        {
            HDC DeviceContext = GetDC(WIndowHandle);

            win32_sound_output SoundOut = {};
            SoundOut.SamplesPerSecond = 48000;
            SoundOut.BytesPerSample = sizeof(int16)*2;
            SoundOut.SecondaryBufferSize = SoundOut.SamplesPerSecond*SoundOut.BytesPerSample;
            SoundOut.LatencySampleCount = 3*(SoundOut.SamplesPerSecond / GameUpdateHz);
            SoundOut.SafetyBytes = ((SoundOut.SamplesPerSecond*SoundOut.BytesPerSample) 
                                    / GameUpdateHz) / 3;

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
            GameMemory.PermanentStorage = VirtualAlloc(0, TotalSize,
                                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
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
            win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = {0};
        
            DWORD LastPlayCursor = 0;
            DWORD LastWriteCursor = 0;
            DWORD AudioLatencyBytes = 0;
            real32 AudioLatencySeconds = 0;

            int64 LastCycleCount = __rdtsc();
            game_input Inp = {};           
            while(RUNNING) 
            {
                win32_game_code GameCode = Win32LoadGameCode();

                game_controller_input *KeyboardController = &Inp.Controllers[0];
                game_controller_input Zero = {};
                for (int i = 0; i < ArrayCount(KeyboardController->Buttons); i++)
                {
                    Zero.Buttons[i].EndedDown = KeyboardController->Buttons[i].EndedDown;
                }
                *KeyboardController = Zero;
                Win32ProcessPendingMessages(KeyboardController);
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
                GameCode.UpdateAndRender(&GameMemory, &OffscreenBuffer, &Inp);

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
                                    (SoundOut.SamplesPerSecond * SoundOut.BytesPerSample)
                                    / GameUpdateHz;
                    
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
                    GameCode.GetSoundSamples(&GameMemory, &SoundBuffer, &Inp);

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
#if INTERNAL_BUILD
                Win32DebugSyncDisplay(&GlobalBackBuffer,
                                    ArrayCount(DebugTimeMarkers), 
                                    DebugTimeMarkers,
                                    DebugTimeMarkersIndex - 1,
                                    &SoundOut, 
                                    TargetSecondsPerFrame);
#endif
                Win32DisplayBufferWindow(
                    &GlobalBackBuffer,
                    DeviceContext,
                    Dimensions.Width, Dimensions.Height);
                // ReleaseDC(WIndowHandle, DeviceContext);
                
#if INTERNAL_BUILD
                {
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                    {
                        win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkersIndex++];
                        if (DebugTimeMarkersIndex >= ArrayCount(DebugTimeMarkers))
                        {
                            DebugTimeMarkersIndex = 0;
                        }
                        Marker->PlayCursor = PlayCursor;
                        Marker->WriteCursor = WriteCursor;
                    }
                }
#endif
                
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
                Win32UnloadGameCode(&GameCode);
            }
        }
        else
        {}
    }
    else
    {}

    return 0;
}