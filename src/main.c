#include "todo.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    HANDLE hMutex = CreateMutexA(NULL, FALSE, "SimpleTodoApp_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWnd = FindWindow("TodoApp", "Todo Application");
        if (hWnd) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        }
        return 0;
    }
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TodoApp";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    hBgBrush = CreateSolidBrush(COLOR_BG);
    wc.hbrBackground = hBgBrush;
    
    RegisterClass(&wc);
    
    hMainWindow = CreateWindow(
        "TodoApp", "Todo Application",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 450,
        NULL, NULL, hInstance, NULL
    );
    
    if (hMainWindow == NULL) {
        MessageBox(NULL, "Window creation failed!", "Error", MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    DeleteObject(hBgBrush);
    return (int)msg.wParam;
}