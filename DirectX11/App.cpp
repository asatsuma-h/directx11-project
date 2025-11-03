#include "App.h"
#include <d3dcompiler.h>

bool D3DApp::Initialize(HWND hWnd, UINT width, UINT height)
{
    mWidth = width;
    mHeight = height;

    // デバイス・スワップチェーン作成
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.OutputWindow = hWnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, mSwapChain.GetAddressOf(), mDevice.GetAddressOf(), nullptr, mContext.GetAddressOf());
    if (FAILED(hr)) return false;

    CreateRenderTargetAndDepth(width, height);
    CreateTriangle();
    CreateShadersAndInputLayout();

    // 定数バッファ作成
    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(ConstantBufferData);
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    mDevice->CreateBuffer(&cbd, nullptr, mConstantBuffer.GetAddressOf());

    // 深度ステンシルステート作成
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = TRUE;                              // 深度テストON
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;     // 深度書き込みON
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;               // Zが小さい(カメラに近い)方を採用
    mDevice->CreateDepthStencilState(&dsDesc, mDepthState.GetAddressOf());
    mContext->OMSetDepthStencilState(mDepthState.Get(), 1);

    return true;
}

void D3DApp::CreateRenderTargetAndDepth(UINT width, UINT height)
{
    ComPtr<ID3D11Texture2D> backBuffer;
    mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, mRTV.GetAddressOf());

    D3D11_TEXTURE2D_DESC dsd{};
    dsd.Width = width;
    dsd.Height = height;
    dsd.MipLevels = 1;
    dsd.ArraySize = 1;
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsd.SampleDesc.Count = 1;
    dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> depthTex;
    mDevice->CreateTexture2D(&dsd, nullptr, depthTex.GetAddressOf());
    mDevice->CreateDepthStencilView(depthTex.Get(), nullptr, mDSV.GetAddressOf());

    mContext->OMSetRenderTargets(1, mRTV.GetAddressOf(), mDSV.Get());

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    mContext->RSSetViewports(1, &vp);
}

void D3DApp::CreateTriangle()
{
    Vertex vertices[] = {
        {{ 0.0f,  0.5f, 0.f}, {0.f, 0.f, -1.f}, {1.f, 0.f, 0.f}}, // 赤
        {{ 0.5f, -0.5f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 1.f, 0.f}}, // 緑
        {{-0.5f, -0.5f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, 1.f}}, // 青
    };
    uint16_t indices[] = {
        0, 1, 2, // 奥
    };

    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = sizeof(vertices);
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit{ vertices };
    mDevice->CreateBuffer(&vbd, &vinit, mVB.GetAddressOf());

    D3D11_BUFFER_DESC ibd{};
    ibd.ByteWidth = sizeof(indices);
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit{ indices };
    mDevice->CreateBuffer(&ibd, &iinit, mIB.GetAddressOf());
}

void D3DApp::CreateShadersAndInputLayout()
{
    ComPtr<ID3DBlob> vsBlob, psBlob, error;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile(
        L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0",
        flags, 0, vsBlob.GetAddressOf(), error.GetAddressOf());
    if (FAILED(hr)) {
        if (error) MessageBoxA(nullptr, (char*)error->GetBufferPointer(), "VS Compile Error", MB_OK);
        else MessageBoxW(nullptr, L"VS: shaders.hlsl not found.", L"VS Compile Error", MB_OK);
        return;
    }

    error.Reset();
    hr = D3DCompileFromFile(
        L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0",
        flags, 0, psBlob.GetAddressOf(), error.GetAddressOf());
    if (FAILED(hr)) {
        if (error) MessageBoxA(nullptr, (char*)error->GetBufferPointer(), "PS Compile Error", MB_OK);
        else MessageBoxW(nullptr, L"PS: shaders.hlsl not found.", L"PS Compile Error", MB_OK);
        return;
    }

    mDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, mVS.GetAddressOf());
    mDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, mPS.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
        D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    mDevice->CreateInputLayout(layout, _countof(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        mInputLayout.GetAddressOf());
}
void D3DApp::Render(float time)
{
    // 定数バッファ更新（回転行列）
    ConstantBufferData cb{};
    XMMATRIX world = XMMatrixRotationY(time);
    XMVECTOR eye = XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
    XMVECTOR at = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up));
    cb.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), (float)mWidth / mHeight, 0.1f, 10.0f));

    // ライトの定数バッファ更新
    cb.lightDir = XMFLOAT3(0.0f, -1.0f, 1.0f);   // 斜め前から照らす光
    cb.lightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    // GPUで情報を送信
    mContext->UpdateSubresource(mConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // クリア
    const float clear[4] = { 0.1f, 0.1f, 0.2f, 1.0f };
    mContext->ClearRenderTargetView(mRTV.Get(), clear);
    mContext->ClearDepthStencilView(mDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // パイプライン設定
    UINT stride = sizeof(Vertex), offset = 0;
    mContext->IASetVertexBuffers(0, 1, mVB.GetAddressOf(), &stride, &offset);
    mContext->IASetIndexBuffer(mIB.Get(), DXGI_FORMAT_R16_UINT, 0);
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->IASetInputLayout(mInputLayout.Get());

    mContext->VSSetShader(mVS.Get(), nullptr, 0);
    mContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
    mContext->PSSetShader(mPS.Get(), nullptr, 0);
    mContext->PSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());

    // 描画
    mContext->DrawIndexed(3, 0, 0);
    mSwapChain->Present(1, 0);
}

void D3DApp::OnResize(UINT width, UINT height)
{
    if (!mSwapChain) return;
    if (width == 0 || height == 0) return;

    mContext->OMSetRenderTargets(0, nullptr, nullptr);
    mRTV.Reset();
    mDSV.Reset();

    // スワップチェーンのサイズ変更
    mSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    // 新しいサイズでRTV / DSV 再生成
    CreateRenderTargetAndDepth(width, height);
}

void D3DApp::Cleanup()
{
    // 順番は重要（Context → SwapChain → Device）
    if (mContext) {
        mContext->ClearState();
        mContext->Flush();
    }

    mVB.Reset();
    mIB.Reset();
    mConstantBuffer.Reset();
    mVS.Reset();
    mPS.Reset();
    mInputLayout.Reset();

    mRTV.Reset();
    mDSV.Reset();
    mSwapChain.Reset();
    mContext.Reset();
    mDevice.Reset();
}