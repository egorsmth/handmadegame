#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint32_t uint32;

global_variable bool RUNNING;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderGradient(int xOffset, int yOffset)
{
    int width = BitmapWidth;
    int Pitch = width*BytesPerPixel;
    uint8 *Row = (uint8 *)BitmapMemory;
    for (int y = 0; y < BitmapHeight; y++)
    {
        uint32 *Pixel = (uint32 *)Row;

        for (int x = 0; x < BitmapWidth; x++)
        {
            uint8 Red = (uint8)(x + xOffset);
            uint8 Green = (uint8)(y + xOffset);
            uint8 Blue = (uint8)(y + yOffset);
            *Pixel = ((Red << 16) | (Green << 8) | Blue);
            ++Pixel;
        }
        Row += Pitch;
    }
}

internal void Win32ResizeDIBSection(int width, int height)
{
    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }
    BitmapWidth = width;
    BitmapHeight = height;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = width;
    BitmapInfo.bmiHeader.biHeight = -height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = BytesPerPixel * width * height;
    BitmapMemory = VirtualAlloc(NULL, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int x, int y, int w, int h) 
{
    int WindowWidth= WindowRect->right - WindowRect->left;
    int WindowHeight= WindowRect->bottom- WindowRect->top;
    StretchDIBits(
        DeviceContext,
        0, 0, BitmapWidth, BitmapHeight,
        0, 0, WindowWidth, WindowHeight,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
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
            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);
            int width = ClientRect.right - ClientRect.left;
            int height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(width, height);
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
            Win32UpdateWindow(DeviceContext, &ClientRectangle, x, y, w, h);
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
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
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
                RenderGradient(xOffset, yOffset);       
                HDC DeviceContext = GetDC(WIndowHandle);
                RECT WindowRect;
                GetClientRect(WIndowHandle, &WindowRect);
                int WindowWidth = WindowRect.right - WindowRect.left;
                int WindowHeight = WindowRect.bottom- WindowRect.top;
                Win32UpdateWindow(DeviceContext, &WindowRect, 0, 0, WindowWidth, WindowHeight);
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