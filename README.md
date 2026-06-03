# DirectX 11 FBX Model Viewer

DirectX 11 / C++ で実装した、FBXモデル表示用のリアルタイムレンダリングサンプルです。  
Windowsネイティブアプリケーションとして、FBX SDKによるモデル読み込み、WICTextureLoaderによるテクスチャ適用、HLSLによる簡易ライティング、カメラ操作、ウィンドウリサイズ対応までを一通り実装しています。

## Demo

<img width="264" height="148" alt="directx_demo" src="https://github.com/user-attachments/assets/7fc94d36-761b-4369-9081-845f2ffa8ef1" />


## Features

- DirectX 11 のデバイス、コンテキスト、スワップチェーン初期化
- レンダーターゲットビューと深度ステンシルビューの作成
- Autodesk FBX SDK を使った FBX メッシュ読み込み
- FBXメッシュの三角形化、頂点座標、法線、UVの取得
- 頂点バッファ、インデックスバッファ、定数バッファの作成
- DirectXTK `WICTextureLoader` によるテクスチャ読み込み
- HLSLによる頂点変換、環境光、拡散反射、鏡面反射
- WASDキーと矢印キーによる簡易カメラ操作
- ウィンドウリサイズ時のバックバッファ、深度バッファ再生成

## Tech Stack

| Category | Technology |
| --- | --- |
| Language | C++20 |
| Graphics API | DirectX 11 |
| Shader | HLSL / Shader Model 5.0 |
| Model Loading | Autodesk FBX SDK |
| Texture Loading | DirectXTK WICTextureLoader |
| Platform | Windows |
| IDE | Visual Studio 2022 |

## Controls

| Key | Action |
| --- | --- |
| `W` / `S` | Move forward / backward |
| `A` / `D` | Move left / right |
| `Left` / `Right` | Rotate camera yaw |
| `Up` / `Down` | Rotate camera pitch |

## Project Structure

```text
.
├── DirectX11.sln
├── README.md
└── DirectX11/
    ├── App.cpp              # DirectX初期化、FBX読み込み、描画処理
    ├── App.h                # D3DAppクラスとGPUリソース定義
    ├── Camera.h             # カメラ移動、回転、View行列生成
    ├── DirectX11.cpp        # Win32エントリーポイント、メインループ
    ├── shaders.hlsl         # 頂点/ピクセルシェーダー
    ├── Assets/
    │   ├── model.fbx
    │   └── MainTexture.png
    └── External/
        └── FBXSDK/
```

## Implementation Notes

### Rendering Pipeline

`D3DApp::Initialize` でDirectX 11の基本リソースを初期化し、`D3DApp::Render` で毎フレーム描画しています。  
描画時はWorld / View / Projection行列、ライト、カメラ位置、マテリアル設定を定数バッファへ転送し、同じFBXモデルを異なるWorld行列で2体描画します。

### FBX Loading

FBX SDKでシーンを読み込み、`FbxGeometryConverter::Triangulate` によってポリゴンを三角形化しています。  
各ポリゴン頂点から位置、法線、UVを取得し、DirectX 11用の頂点バッファとインデックスバッファへ変換しています。

### Shader

`shaders.hlsl` では、頂点シェーダーで座標変換とワールド空間情報の受け渡しを行い、ピクセルシェーダーでテクスチャカラー、環境光、拡散反射、鏡面反射を合成しています。

### Resource Management

DirectX COMオブジェクトは `Microsoft::WRL::ComPtr` で管理し、終了時には `Cleanup` で明示的にリソースを解放しています。  
ウィンドウリサイズ時には既存のRTV / DSVを破棄し、スワップチェーンのバッファサイズ変更後に再生成します。

## Build

### Requirements

- Windows 10 SDK
- Visual Studio 2022
- MSVC v143
- DirectXTK
- Autodesk FBX SDK

### Steps

1. `DirectX11.sln` を Visual Studio 2022 で開く
2. 構成を `Debug`、プラットフォームを `x64` に設定する
3. `DirectX11` プロジェクトをビルドする
4. 実行ディレクトリから `Assets/model.fbx` と `Assets/MainTexture.png` を参照できる状態で起動する

## What This Project Demonstrates

- DirectX 11の基本的な描画パイプラインをC++で構築できること
- GPUリソース、シェーダー、定数バッファの役割を理解して実装できること
- 外部アセットを読み込み、描画用データへ変換できること
- Win32アプリケーションのメインループとリアルタイム描画を接続できること
- カメラ、ライト、マテリアルなど3D描画に必要な要素を分離して扱えること
