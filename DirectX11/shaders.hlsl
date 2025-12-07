cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    
    // ライトと環境光
    float3 lightDir;        // 光の方向
    float lightIntensity;   // 強度
    float4 lightColor;      // 拡散/鏡面に掛ける光色
    float4 ambientColor;      // 環境光色
    
    // カメラ & マテリアル
    float3 camPos;          // 視線ベクトル用にPSで使用
    float specPower;        // 鏡面の鋭さ(32, 64, 128など)
    float4 materialColor;   // アルベド乗算色
    uint useTexture;        // 1: テクスチャ使用 / 0: 未使用
    float3 _pad;            // 16byte合わせ
}

// テクスチャとサンプラー
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

// 頂点構造体（入力）
struct VSIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

// 頂点構造体（出力）
struct VSOut
{
    float4 pos : SV_POSITION;
    float3 nW : NORMAL;         // ワールド空間法線
    float2 uv : TEXCOORD;
    float3 posW : TEXCOORD1;    // ワールド位置
};

// 頂点シェーダー
VSOut VSMain(VSIn i)
{
    VSOut o;
    
    // モデル　-> ワールド
    float4 wpos = mul(float4(i.pos, 1.0), world);
    o.posW = wpos.xyz;
    
    // ワールド -> ビュー
    float4 vpos = mul(wpos, view);
    // ビュー -> プロジェクション
    o.pos = mul(vpos, proj);
    
    // 法線をワールド空間へ変換
    o.nW = mul(i.normal, (float3x3) transpose(world));
    
    o.uv = i.uv;
    
    return o;
}

// ピクセルシェーダー
float4 PSMain(VSOut i) : SV_TARGET
{
    // 正規化
    float3 N = normalize(i.nW);
    float3 L = normalize(-lightDir);
    float V = normalize(camPos - i.posW);
    float3 H = normalize(L + V);
    
    // 基本のBRDF項
    float NdotL = saturate(dot(N, L));
    float diff = NdotL;
    
    float NdotH = saturate(dot(N, H));
    float spec = pow(NdotH, max(specPower, 1.0));
    
    // アルベド
    float4 albedo = materialColor;
    if (useTexture != 0)
    {
        float4 texColor = tex0.Sample(samp0, i.uv);
        albedo *= texColor;
    }
    
    // 環境光 + 拡散 + 鏡面
    float3 ambient = ambientColor.rgb * albedo.rgb;
    float3 diffuse = lightIntensity * diff * lightColor.rgb * albedo.rgb;
    float3 specular = lightIntensity * spec * lightColor.rgb; // 金属度なしのシンプル仕様

    float3 color = ambient + diffuse + specular;

    return float4(color, albedo.a);
}
