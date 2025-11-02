#include <windows.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hWnd, &ps);
        HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdc, &ps.rcPaint, black); // クライアント領域を「真っ黒」で塗る
        EndPaint(hWnd, &ps);
    } return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t* kClassName = L"Dx11LearnStage1Window";

    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
    wc.style = CS_OWNDC;              // 将来のDX描画でも無駄がない設定
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // WM_PAINTで黒塗りするので実質未使用
    wc.lpszClassName = kClassName;

    if (!RegisterClassExW(&wc)) return -1;

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc{ 0, 0, 1280, 720 };
    AdjustWindowRect(&rc, style, FALSE);

    HWND hWnd = CreateWindowExW(
        0, kClassName, L"Stage1 - Black Window",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);

    if (!hWnd) return -1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 標準メッセージループ
    MSG msg{};
    while (true) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else {
            // 将来的にはここで「フレーム更新・描画」を回す（DX初期化後）
            Sleep(1); // ひとまずCPUを休ませる
        }
    }
    return static_cast<int>(msg.wParam);
}
