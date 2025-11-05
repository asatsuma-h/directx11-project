#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <cstdint>
#include <string>
#include <vector>

#pragma comment(lib, "d3d11.lib")       // D3D11 の本体
#pragma comment(lib, "dxgi.lib")        // スワップチェーンなど
#pragma comment(lib, "d3dcompiler.lib") // シェーダーコンパイル用
#pragma comment(lib, "libfbxsdk.lib")	// FBX SDK

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Direct3D管理クラス
class D3DApp
{
public:
	bool Initialize(HWND hWnd, UINT width, UINT height);
	void Render(float time);
	void OnResize(UINT width, UINT height);
	void Cleanup();

private:
	void CreateRenderTargetAndDepth(UINT width, UINT height);
	void CreateTriangle();
	void CreateShadersAndInputLayout();
	bool LoadFBXModel(const std::string& path);

private:
	UINT mWidth = 1280;
	UINT mHeight = 720;

	ComPtr<ID3D11Device> mDevice;
	ComPtr<ID3D11DeviceContext> mContext;
	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D11RenderTargetView> mRTV;
	ComPtr<ID3D11DepthStencilView> mDSV;
	ComPtr<ID3D11DepthStencilState> mDepthState;	// 深度ステート

	ComPtr<ID3D11Buffer> mVB;
	ComPtr<ID3D11Buffer> mIB;
	ComPtr<ID3D11Buffer> mConstantBuffer;
	ComPtr<ID3D11VertexShader> mVS;
	ComPtr<ID3D11PixelShader> mPS;
	ComPtr<ID3D11InputLayout> mInputLayout;

	struct Vertex 
	{ 
		XMFLOAT3 pos; 
		XMFLOAT3 normal;
		XMFLOAT2 uv;
	};

	struct ConstantBufferData
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX proj;
		XMFLOAT3 lightDir;		// 光の方向
		float pad1;				// 16バイト境界用パディング
		XMFLOAT4 lightColor;	// 光の色
	};

	UINT mIndexCount = 0;		// FBX読み込み後のインデックス数
};
