#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

global_variable real32 win32_PI = 3.14159265358979323846;

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

global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

internal void LoadXInput(void)
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
            BufferDesc.dwFlags = 0;
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

global_variable bool RUNNING;

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dims {
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    int ToneHz;
    int16 ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
};

internal void Wind32FillSoundBuffer(
    win32_sound_output *SoundOutput,
    DWORD ByteToLock,
    DWORD BytesToWrite)
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
        int16 *SampleOut = (int16 *)Region1;
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            real32 t = 2.0f * win32_PI * (real32)SoundOutput->RunningSampleIndex++ / (real32)SoundOutput->WavePeriod;
            real32 sineValue = sinf(t);
            int16 SampleValue = (int16)(sineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
        }
        SampleOut = (int16 *)Region2;
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            real32 t = 2.0f * win32_PI * (real32)SoundOutput->RunningSampleIndex++ / (real32)SoundOutput->WavePeriod;
            real32 sineValue = sinf(t);
            int16 SampleValue = (int16)(sineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
        }
        GlobalSecondaryBuffer->Unlock(
            Region1, Region1Size,
            Region2, Region2Size);
    }
}

internal void RenderGradient(
    win32_offscreen_buffer *Buffer, 
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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKcode = wParam;
            bool wasDown = ((lParam & (1 << 30)) != 0);
            bool isDown = ((lParam & (1 << 31)) == 0);
            
            if (isDown == wasDown) break;
            
            if (VKcode == 'W')
            {}
            else if (VKcode == 'A')
            {}
            else if (VKcode == 'S')
            {}
            else if (VKcode == 'D')
            {}
            else if (VKcode == 'E')
            {}
            else if (VKcode == 'Q')
            {}
            else if (VKcode == VK_UP)
            {}
            else if (VKcode == VK_LEFT)
            {}
            else if (VKcode == VK_DOWN)
            {}
            else if (VKcode == VK_RIGHT)
            {}
            else if (VKcode == VK_SPACE)
            {}
            else if (VKcode == VK_ESCAPE)
            {
                RUNNING = false;
            }
        } break;
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
            // OutputDebugStringA("default\n");
            Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
            break;
    }
    return Result;
}

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
{
    LoadXInput();
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);

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

            int xOffset = 0;
            int yOffset = 0;

            win32_sound_output SoundOut = {};
            SoundOut.SamplesPerSecond = 48000;
            SoundOut.ToneHz = 256;
            SoundOut.ToneVolume = 1000;
            SoundOut.RunningSampleIndex = 0;
            SoundOut.WavePeriod = SoundOut.SamplesPerSecond/SoundOut.ToneHz;
            SoundOut.BytesPerSample = sizeof(int16)*2;
            SoundOut.SecondaryBufferSize = SoundOut.SamplesPerSecond*SoundOut.BytesPerSample;

            Win32InitDSound(
                WIndowHandle, 
                SoundOut.SamplesPerSecond,
                SoundOut.SecondaryBufferSize);

            Wind32FillSoundBuffer(&SoundOut, 0, SoundOut.SecondaryBufferSize);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            RUNNING = true;

            LARGE_INTEGER EndPerfCounter;
            LARGE_INTEGER StartPerfCounter;
            QueryPerformanceCounter(&StartPerfCounter);
            int64 LastCucleCount = __rdtsc();
            while(RUNNING) 
            {
               
                MSG Message;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        RUNNING = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
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
                
                RenderGradient(&GlobalBackBuffer, xOffset, yOffset);       

                DWORD PlayCursor;
                DWORD WriteCursor;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = SoundOut.RunningSampleIndex*SoundOut.BytesPerSample % SoundOut.SecondaryBufferSize;
                    DWORD BytesToWrite;
                    if (ByteToLock > PlayCursor) 
                    {
                        BytesToWrite = (SoundOut.SecondaryBufferSize - ByteToLock) + PlayCursor;
                    }
                    else if (ByteToLock == PlayCursor)
                    {
                        BytesToWrite = 0;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    
                    Wind32FillSoundBuffer(&SoundOut, ByteToLock, BytesToWrite);
                }

                win32_window_dims Dimensions = Win32GetWindowDims(WIndowHandle);
                Win32DisplayBufferWindow(
                    &GlobalBackBuffer,
                    DeviceContext,
                    Dimensions.Width, Dimensions.Height);
                ReleaseDC(WIndowHandle, DeviceContext);
                ++xOffset;

                QueryPerformanceCounter(&EndPerfCounter);
                int64 CounterElapsed = EndPerfCounter.QuadPart - StartPerfCounter.QuadPart;
                int32 MsPerFrame = (int32)((1000*CounterElapsed) / PerfFreq.QuadPart);
                int32 FPS = (int32)(PerfFreq.QuadPart / CounterElapsed);

                int64 EndCucleCount = __rdtsc();
                int32 CyclesElapsed = (int32)((EndCucleCount - LastCucleCount) / (1000 * 1000));
                char Buffer[256];
                wsprintf(Buffer, "MS/frame: %d ms - FPS: %d - MegaCycles: %d\n", MsPerFrame, FPS, CyclesElapsed);
                OutputDebugStringA(Buffer);
                StartPerfCounter = EndPerfCounter;
                LastCucleCount = EndCucleCount;
            }
        }
        else
        {}
    }
    else
    {}

    return 0;
}