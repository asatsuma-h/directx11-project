#include <windows.h>
#include "App.h"

D3DApp gApp;
UINT gWidth = 1280, gHeight = 720;
HWND g_hWnd;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_SIZE:
        gWidth = LOWORD(lp);
        gHeight = HIWORD(lp);
        if (wp != SIZE_MINIMIZED) gApp.OnResize(gWidth, gHeight);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t* clsName = L"D3D11Window";
    WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = clsName;
    RegisterClassEx(&wc);

    RECT rc{ 0,0,(LONG)gWidth,(LONG)gHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindowEx(0, clsName, L"Step3 - Matrix Transform",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(g_hWnd, nCmdShow);

    gApp.Initialize(g_hWnd, gWidth, gHeight);

    MSG msg{};
    float t = 0.0f;
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            t += 0.01f;

            // カメラ操作
            if (GetAsyncKeyState('W') & 0x8000) gApp.mCamera.Move(0, 0, 0.02f);
            if (GetAsyncKeyState('S') & 0x8000) gApp.mCamera.Move(0, 0, -0.02f);
            if (GetAsyncKeyState('A') & 0x8000) gApp.mCamera.Move(-0.02f, 0, 0);
            if (GetAsyncKeyState('D') & 0x8000) gApp.mCamera.Move(0.02f, 0, 0);
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) gApp.mCamera.Rotate(-0.02f, 0);
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) gApp.mCamera.Rotate(0.02f, 0);
            if (GetAsyncKeyState(VK_UP) & 0x8000) gApp.mCamera.Rotate(0, 0.02f);
            if (GetAsyncKeyState(VK_DOWN) & 0x8000) gApp.mCamera.Rotate(0, -0.02f);

            gApp.Render(t);
        }
    }
    gApp.Cleanup();
    return 0;
}