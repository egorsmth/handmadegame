#include <windows.h>

LRESULT CALLBACK MainWindowCallback(
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
            OutputDebugStringA("WM_DESTROY\n");
        }
            break;
        case WM_CLOSE:
        {
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
            static DWORD Operation = WHITENESS;
            
            PatBlt(DeviceContext, x, y, w, h, Operation);
            if (Operation == WHITENESS)
            {
                Operation = BLACKNESS;
            } else
            {
                Operation = WHITENESS;
            }
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
    WindowClass.lpfnWndProc = MainWindowCallback;
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
            
            for(;;) 
            {
                MSG Message;
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else
                {
                    break;
                }
                
            }
            
            
        }
        else
        {}
    }
    else
    {}

    return 0;
}