#include "App.h"
#include <d3dcompiler.h>
#include <fbxsdk.h>      // FBX SDKメインヘッダー
#include <vector>
#include <string>
#include <iostream>
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
    //CreateTriangle();
    LoadFBXModel("Assets/model.fbx");
    LoadTexture(L"Assets/MainTexture.png");
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

bool D3DApp::LoadFBXModel(const std::string& path)
{
    // FBXマネージャ生成
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    // インポーター生成
    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(path.c_str(), -1, manager->GetIOSettings()))
    {
        MessageBoxA(nullptr, importer->GetStatus().GetErrorString(), "FBX Import Error", MB_OK);
        importer->Destroy();
        manager->Destroy();
        return false;
    }

    // シーン生成・読み込み
    FbxScene* scene = FbxScene::Create(manager, "scene");
    importer->Import(scene);
    importer->Destroy();

    // 三角形化
    FbxGeometryConverter converter(manager);
    converter.Triangulate(scene, true);

    // 最初のメッシュを探す
    FbxNode* root = scene->GetRootNode();
    if (!root)
    {
        manager->Destroy();
        return false;
    }

    FbxMesh* mesh = nullptr;
    for (int i = 0; i < root->GetChildCount(); i++) {
        FbxNode* node = root->GetChild(i);
        if (node->GetMesh()) {
            mesh = node->GetMesh();
            break;
        }
    }
    if (!mesh)
    {
        manager->Destroy();
        return false;
    }

    // 頂点データ格納
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    int polyCount = mesh->GetPolygonCount();
    for (int p = 0; p < polyCount; p++)
    {
        // 常に三角形
        for (int v = 0; v < 3; v++)
        {
            Vertex vert{};

            int ctrlIdx = mesh->GetPolygonVertex(p, v);

            // 位置
            FbxVector4 pos = mesh->GetControlPointAt(ctrlIdx);
            vert.pos = { (float)pos[0], (float)pos[1], (float)pos[2] };

            // 法線
            FbxVector4 normal;
            mesh->GetPolygonVertexNormal(p, v, normal);
            vert.normal = { (float)normal[0], (float)normal[1], (float)normal[2] };

            // UV
            FbxStringList uvNames;
            mesh->GetUVSetNames(uvNames);
            if (uvNames.GetCount() > 0)
            {
                const char* uvName = uvNames[0];
                FbxVector2 uv;
                bool unmapped;
                if (mesh->GetPolygonVertexUV(p, v, uvName, uv, unmapped))
                {
                    vert.uv = { (float)uv[0], 1.0f - (float)uv[1] };
                }
            }
            vertices.push_back(vert);
            indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
        }
    }

    // --- DirectX バッファ作成 ---
    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = UINT(sizeof(Vertex) * vertices.size());
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit{ vertices.data() };
    HRESULT hr = mDevice->CreateBuffer(&vbd, &vinit, mVB.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"頂点バッファ作成失敗", L"Error", MB_OK);
        manager->Destroy();
        return false;
    }

    D3D11_BUFFER_DESC ibd{};
    ibd.ByteWidth = UINT(sizeof(uint32_t) * indices.size());
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit{ indices.data() };
    hr = mDevice->CreateBuffer(&ibd, &iinit, mIB.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"インデックスバッファ作成失敗", L"Error", MB_OK);
        manager->Destroy();
        return false;
    }

    mIndexCount = static_cast<UINT>(indices.size());

    // --- 後処理 ---
    manager->Destroy();
    return true;
}

void D3DApp::LoadTexture(const std::wstring& path)
{
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        mDevice.Get(),
        mContext.Get(),
        path.c_str(),
        nullptr,
        mTextureSRV.GetAddressOf()
    );

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"テクスチャ読み込み失敗", L"Error", MB_OK);
        return;
    }

    // サンプラー（補間設定）
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samp.MinLOD = 0;
    samp.MaxLOD = D3D11_FLOAT32_MAX;

    mDevice->CreateSamplerState(&samp, mSamplerState.GetAddressOf());
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
        {{ 0.0f,  0.5f, 0.f}, {0.f, 0.f, -1.f}, {1.f, 0.f}}, // 赤
        {{ 0.5f, -0.5f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 1.f}}, // 緑
        {{-0.5f, -0.5f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 0.f}}, // 青
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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    mDevice->CreateInputLayout(layout, _countof(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        mInputLayout.GetAddressOf());
}
void D3DApp::Render(float time)
{
    ConstantBufferData cb{};

    XMVECTOR eye = XMVectorSet(0.0f, 0.7f, -3.0f, 0.0f);
    XMVECTOR at = XMVectorSet(0.0f, 0.2f, 0.0f, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = mCamera.GetViewMatrix();
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), (float)mWidth / (float)mHeight, 0.1f, 100.0f);

    // 光源設定
    cb.lightDir = XMFLOAT3(0.2f, -1.0f, 0.3f); // 方向（正規化でなくてもPSで正規化）
    cb.lightIntensity = 1.0f;
    cb.lightColor = XMFLOAT4(1, 1, 1, 1);
    cb.ambientColor = XMFLOAT4(0.06f, 0.06f, 0.07f, 1.0f); // ほんのり青み
    cb.view = XMMatrixTranspose(view);
    cb.proj = XMMatrixTranspose(proj);

    // カメラ位置（eye を入れる）
    cb.camPos = mCamera.GetPosition();

    const float clear[4] = { 0.05f, 0.05f, 0.1f, 1.0f };
    mContext->ClearRenderTargetView(mRTV.Get(), clear);
    mContext->ClearDepthStencilView(mDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // パイプライン設定
    UINT stride = sizeof(Vertex), offset = 0;
    mContext->IASetVertexBuffers(0, 1, mVB.GetAddressOf(), &stride, &offset);
    mContext->IASetIndexBuffer(mIB.Get(), DXGI_FORMAT_R32_UINT, 0);
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->IASetInputLayout(mInputLayout.Get());
    mContext->VSSetShader(mVS.Get(), nullptr, 0);
    mContext->PSSetShader(mPS.Get(), nullptr, 0);
    mContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
    mContext->PSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
    mContext->PSSetShaderResources(0, 1, mTextureSRV.GetAddressOf());
    mContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

    // マテリアル
    cb.materialColor = XMFLOAT4(1, 1, 1, 1); // アルベド乗算（白＝無加工）
    cb.specPower = 64.0f;                // 鏡面の鋭さ
    cb.useTexture = 1u;                   // テクスチャを使う

    // --- モデル1 ---
    XMMATRIX world1 = XMMatrixScaling(0.5f, 0.5f, 0.5f)
        * XMMatrixTranslation(-1.0f, 0.0f, 0.0f)
        * XMMatrixRotationY(time);
    cb.world = XMMatrixTranspose(world1);
    mContext->UpdateSubresource(mConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    mContext->DrawIndexed(mIndexCount, 0, 0);

    // --- モデル2 ---
    XMMATRIX world2 = XMMatrixScaling(0.5f, 0.5f, 0.5f)
        * XMMatrixTranslation(1.0f, 0.0f, 0.0f)
        * XMMatrixRotationY(-time * 0.5f);
    cb.world = XMMatrixTranspose(world2);
    mContext->UpdateSubresource(mConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    mContext->DrawIndexed(mIndexCount, 0, 0);

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