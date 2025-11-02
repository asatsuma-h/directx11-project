#include <windows.h>                    // Win32 API に必要
#include <d3d11.h>                      // Direct3D 11 の主要インターフェース
#include <d3dcompiler.h>                // HLSL シェーダーコンパイルに必要
#include <cstdint>
#pragma comment(lib, "d3d11.lib")       // D3D11 の本体
#pragma comment(lib, "dxgi.lib")        // スワップチェーンなど
#pragma comment(lib, "d3dcompiler.lib") // シェーダーコンパイル用

HWND g_hWnd{};                                  // ウィンドウハンドル
UINT g_Width = 1280, g_Height = 720;            // 画面幅、高さ

ID3D11Device* g_Device = nullptr;               // デバイス本体
ID3D11DeviceContext* g_Context = nullptr;       // 即時コンテキスト
IDXGISwapChain* g_SwapChain = nullptr;          // スワップチェーン（BackBuffer制御）
ID3D11RenderTargetView* g_RTV = nullptr;        // レンダーターゲットビュー（描画先）
ID3D11Texture2D* g_DepthTex = nullptr;          // 深度ステンシル用テクスチャ
ID3D11DepthStencilView* g_DSV = nullptr;        // 深度ステンシルビュー
D3D_FEATURE_LEVEL g_FeatureLevel{};

// 描画用リソース
struct Vertex { float pos[3]; float color[3]; };
ID3D11Buffer* g_VB = nullptr;
ID3D11Buffer* g_IB = nullptr;
ID3D11VertexShader* g_VS = nullptr;
ID3D11PixelShader* g_PS = nullptr;
ID3D11InputLayout* g_InputLayout = nullptr;

/*
* バックバッファからRenderTargetViewとDepthStencilを作成
*/
void CreateRTVAndDSV(UINT width, UINT height)
{
    // 既存リソースを開放
    if (g_Context) g_Context->OMSetRenderTargets(0, nullptr, nullptr);
    if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
    if (g_DSV) { g_DSV->Release(); g_DSV = nullptr; }
    if (g_DepthTex) { g_DepthTex->Release(); g_DepthTex = nullptr; }

    // BackBuffeからRTVを作成
    ID3D11Texture2D* backBuffer = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RTV);
    backBuffer->Release();

    // 深度ステンシルバッファを作成
    D3D11_TEXTURE2D_DESC dsd{};
    dsd.Width = width;
    dsd.Height = height;
    dsd.MipLevels = 1;
    dsd.ArraySize = 1;
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24bit＋ステンシル8bit
    dsd.SampleDesc.Count = 1;
    dsd.Usage = D3D11_USAGE_DEFAULT;
    dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    g_Device->CreateTexture2D(&dsd, nullptr, &g_DepthTex);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
    dsvd.Format = dsd.Format;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    g_Device->CreateDepthStencilView(g_DepthTex, &dsvd, &g_DSV);

    // 出力ターゲットとしてセット
    g_Context->OMSetRenderTargets(1, &g_RTV, g_DSV);

    // ビューポート
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    g_Context->RSSetViewports(1, &vp);
}

/*
* Direct3D11 初期化
*/
bool InitD3D(HWND hWnd, UINT width, UINT height)
{
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2; // ダブルバッファ
    scd.OutputWindow = hWnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // 学習シンプル版（Windows10+なら FLIP 系推奨）

    UINT flags = 0;
#if _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[]{
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    };

    // デバイスとスワップチェーンを作成
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                     // 使用するGPUアダプタ（nullptrで自動選択）
        D3D_DRIVER_TYPE_HARDWARE,    // ハードウェアGPUを使用
        nullptr,                     // ソフトウェアレンダラ不要
        flags,                       // デバッグフラグなど
        levels, _countof(levels),    // 対応する機能レベルの配列
        D3D11_SDK_VERSION,           // SDKバージョン（固定）
        &scd,                        // スワップチェーン設定
        &g_SwapChain,                // スワップチェーンが返る
        &g_Device,                   // デバイスが返る
        &g_FeatureLevel,             // 実際に使用されたFeatureLevel
        &g_Context                   // デバイスコンテキスト（描画命令用）
    );
    if (FAILED(hr)) return false;

    // レンダーターゲットと深度バッファを作成
    CreateRTVAndDSV(width, height);
    return true;
}

/*
* 頂点・インデックスバッファ作成
*/
void CreateTriangleBuffers()
{
    // NDC座標（-1〜1範囲内）で三角形を定義
    Vertex vertices[] = {
        {{ 0.0f,  0.5f, 0.f}, {1.f, 0.f, 0.f}}, // 上（赤）
        {{ 0.5f, -0.5f, 0.f}, {0.f, 1.f, 0.f}}, // 右下（緑）
        {{-0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}}, // 左下（青）
    };

    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = sizeof(vertices);
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit{};
    vinit.pSysMem = vertices;
    g_Device->CreateBuffer(&vbd, &vinit, &g_VB);

    // 頂点インデックス
    uint16_t indices[] = { 0, 1, 2 };
    D3D11_BUFFER_DESC ibd{};
    ibd.ByteWidth = sizeof(indices);
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit{};
    iinit.pSysMem = indices;
    g_Device->CreateBuffer(&ibd, &iinit, &g_IB);
}

/*
* シェーダー作成
*/
void CreateShadersAndInputLayout()
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompileFromFile(
        L"shaders.hlsl", nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "Vertex Shader Compile Error", MB_OK);
            errorBlob->Release();
        }
        else {
            MessageBoxW(nullptr, L"shaders.hlsl が見つかりません。", L"Shader Compile Error", MB_OK);
        }
        return;
    }

    // ピクセルシェーダー
    D3DCompileFromFile(L"shaders.hlsl", 
        nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }

    g_Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_VS);
    g_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_PS);

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    g_Device->CreateInputLayout(
        ied, _countof(ied),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        &g_InputLayout);

    vsBlob->Release();
    psBlob->Release();
}

/*
* 三角形描画
*/
void RenderTriangle()
{
    const float clear[4] = { 0.1f, 0.1f, 0.25f, 1.0f };
    g_Context->ClearRenderTargetView(g_RTV, clear);
    g_Context->ClearDepthStencilView(g_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    UINT stride = sizeof(Vertex), offset = 0;
    g_Context->IASetVertexBuffers(0, 1, &g_VB, &stride, &offset);
    g_Context->IASetIndexBuffer(g_IB, DXGI_FORMAT_R16_UINT, 0);
    g_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_Context->IASetInputLayout(g_InputLayout);

    g_Context->VSSetShader(g_VS, nullptr, 0);
    g_Context->PSSetShader(g_PS, nullptr, 0);
    g_Context->OMSetRenderTargets(1, &g_RTV, g_DSV);

    g_Context->DrawIndexed(3, 0, 0);
    g_SwapChain->Present(1, 0);
}

/*
* 背景をクリア
*/
void RenderClear()
{
    const float clear[4]{ 0.392f, 0.584f, 0.929f, 1.0f }; // CornflowerBlue
    g_Context->ClearRenderTargetView(g_RTV, clear);
    g_Context->ClearDepthStencilView(g_DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    g_SwapChain->Present(1, 0); // バックバッファを前面に表示
}

/*
* WM_SIZEイベントで再生成
*/
void OnResize(UINT width, UINT height)
{
    if (!g_SwapChain) return;
    if (width == 0 || height == 0) return;

    // 既存リソースの解放
    g_Context->OMSetRenderTargets(0, nullptr, nullptr);
    if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
    if (g_DSV) { g_DSV->Release(); g_DSV = nullptr; }
    if (g_DepthTex) { g_DepthTex->Release(); g_DepthTex = nullptr; }

    // スワップチェーンのサイズ変更
    g_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    // 新しいサイズで再生成
    CreateRTVAndDSV(width, height);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        g_Width = LOWORD(lParam);
        g_Height = HIWORD(lParam);
        // ウィンドウ最小化の場合は実行しない
        if (wParam != SIZE_MINIMIZED) OnResize(g_Width, g_Height);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    // ウィンドウクラス登録
    const wchar_t* kClassName = L"Dx11LearnStage1Window";

    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;              // 将来のDX描画でも無駄がない設定
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // WM_PAINTで黒塗りするので実質未使用
    wc.lpszClassName = kClassName;

    if (!RegisterClassExW(&wc)) return -1;

    // ウィンドウ作成
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc{ 0, 0, (LONG)g_Width, (LONG)g_Height};
    AdjustWindowRect(&rc, style, FALSE);

    g_hWnd = CreateWindowExW(
        0, kClassName, L"Stage1 - Black Window",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);

    if (!g_hWnd) return -1;

    ShowWindow(g_hWnd, nCmdShow);

    // Direct3D11 初期化
    if (!InitD3D(g_hWnd, g_Width, g_Height)) return -1;

    // 三角形関連の初期化
    CreateTriangleBuffers();
    CreateShadersAndInputLayout();

    UpdateWindow(g_hWnd);

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
            RenderTriangle();
            //Sleep(1); // ひとまずCPUを休ませる
        }
    }
    return static_cast<int>(msg.wParam);
}
