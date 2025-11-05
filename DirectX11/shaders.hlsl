cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    float3 lightDir;        // 光の方向
    float pad;
    float4 lightColor;      // 光の色
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
    float4 pos : SV_POSITION; // 大文字に注意！
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

// 頂点シェーダー
VSOut VSMain(VSIn i)
{
    VSOut o;
    
    // モデル　-> ワールド
    float4 wpos = mul(float4(i.pos, 1.0), world);
    // ワールド -> ビュー
    float4 vpos = mul(wpos, view);
    // ビュー -> プロジェクション
    o.pos = mul(vpos, proj);
    
    // 法線をワールド空間へ変換
    o.normal = mul(i.normal, (float3x3) transpose(world));
    
    o.uv = i.uv;
    
    return o;
}

// ピクセルシェーダー
float4 PSMain(VSOut i) : SV_TARGET
{
    float3 N = normalize(i.normal);
    float3 L = normalize(-lightDir);
    float diff = saturate(dot(N, L));
    
    float4 texColor = tex0.Sample(samp0, i.uv);
    float3 litColor = texColor.rgb * lightColor.rgb * diff;
    return tex0.Sample(samp0, i.uv);
}
