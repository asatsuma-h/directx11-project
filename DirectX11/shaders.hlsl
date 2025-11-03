cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
}

// 頂点構造体（入力）
struct VSIn
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

// 頂点構造体（出力）
struct VSOut
{
    float4 pos : SV_POSITION; // 大文字に注意！
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
    o.col = i.col;
    
    return o;
}

// ピクセルシェーダー
float4 PSMain(VSOut i) : SV_TARGET
{
    return float4(i.col * 2.0, 1.0);
}
