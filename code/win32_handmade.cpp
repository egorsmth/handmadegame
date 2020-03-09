#include <windows.h>
#include <stdint.h>
#include <Xinput.h>

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

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {return 0;}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVIbration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {return 0;}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
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

struct windowDims {
    int Width;
    int Height;
};

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

internal windowDims Win32GetWindowDims(HWND WindowHandler)
{
    HDC DeviceContext = GetDC(WindowHandler);
    RECT WindowRect;
    GetClientRect(WindowHandler, &WindowRect);
    windowDims Result;
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
            windowDims Dimensions = Win32GetWindowDims(hwnd);
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
            int xOffset = 0;
            int yOffset = 0;
            HDC DeviceContext = GetDC(WIndowHandle);
            RUNNING = true;
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
                windowDims Dimensions = Win32GetWindowDims(WIndowHandle);
                Win32DisplayBufferWindow(
                    &GlobalBackBuffer,
                    DeviceContext,
                    Dimensions.Width, Dimensions.Height);
                ReleaseDC(WIndowHandle, DeviceContext);
                ++xOffset;
            }
        }
        else
        {}
    }
    else
    {}

    return 0;
}