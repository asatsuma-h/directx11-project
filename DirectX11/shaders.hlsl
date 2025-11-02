// shaders.hlsl - Step2: 三角形描画テスト用
// --------------------------------------------

// 頂点構造体（入力）
struct VSIn
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

// 頂点構造体（出力）
struct VSOut
{
    float4 svpos : SV_POSITION; // 大文字に注意！
    float3 col : COLOR;
};

// 頂点シェーダー
VSOut VSMain(VSIn i)
{
    VSOut o;
    o.svpos = float4(i.pos, 1.0);
    o.col = i.col;
    return o;
}

// ピクセルシェーダー
float4 PSMain(VSOut i) : SV_TARGET
{
    return float4(i.col, 1.0);
}
