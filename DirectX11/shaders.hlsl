cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    float3 lightDir;        // 光の方向
    float pad;
    float4 lightColor;      // 光の色
}

// 頂点構造体（入力）
struct VSIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 col : COLOR;
};

// 頂点構造体（出力）
struct VSOut
{
    float4 pos : SV_POSITION; // 大文字に注意！
    float3 normal : NORMAL;
    float3 col : COLOR;
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
    
    o.col = i.col;
    
    return o;
}

// ピクセルシェーダー
float4 PSMain(VSOut i) : SV_TARGET
{
    float3 N = normalize(i.normal);
    float3 L = normalize(-lightDir);
    float diff = saturate(dot(N, L));
    float3 litColor = i.col * lightColor.rgb * diff;
    return float4(litColor, 1.0);
}
