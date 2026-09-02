#include "Common.hlsli"
#include "../LightElement.hlsli"

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

struct MaterialData
{
    float2 PlayerPos; // プレイヤーの位置
    float time; // 時間
    float radius; // 半径
    
    float swirl; // 渦
    float scale; // サイズ
    float strength;
    float pad;
    
    float2 UniversePos;
    float2 pad1;
};

static const uint VERTEX_STRIDE = 52;

VertexData GetHitVertex(MyAttribute attrib, uint vertexHandle, uint indexHandle)
{
    uint start = PrimitiveIndex() * 3;
    
    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];
    float4 tangents[3];

    for (int i = 0; i < 3; ++i)
    {
        uint index = gBufferData[indexHandle].Load <
        uint > ((start + i) * 4);
        
        VertexData v = gBufferData[vertexHandle].Load < VertexData > (index * VERTEX_STRIDE);
        
        positions[i] = v.position.xyz;
        normals[i] = v.normal;
        texcoords[i] = v.texcoord;
        tangents[i] = v.tangent;
    }
    
    VertexData v = (VertexData) 0;
    v.position.xyz = CalcHitAttribute3(positions, attrib.barys);
    v.position.w = 1.0f;
    v.texcoord = CalcHitAttribute2(texcoords, attrib.barys);
    v.normal = CalcHitAttribute3(normals, attrib.barys);
    v.normal = normalize(v.normal);
    v.tangent = CalcHitAttribute4(tangents, attrib.barys);
    return v;
}

// 宇宙
float3 Universe(float2 uv, float iTime)
{
    float len = length(uv);
 
    // 範囲外は完全に黒。90回ループを回すだけ無駄なので早期に返す
    if (len > 0.95f)
        return float3(0, 0, 0);
 
    float t = iTime * .1 + ((.25 + .05 * sin(iTime * .1)) / (len + .07)) * 2.2;
    float si = sin(t);
    float co = cos(t);
    float2x2 ma = float2x2(co, si, -si, co);
 
    float v1 = 0.0, v2 = 0.0, v3 = 0.0;
    float s = 0.0;
 
    [loop]
    for (int i = 0; i < 90; i++)
    {
        float3 p = s * float3(uv, 0.0);
        p.xy = mul(p.xy, ma);
        p += float3(.22, .3, s - 1.5 - sin(iTime * .13) * .1);
 
        [loop]
        for (int j = 0; j < 8; j++)
            p = abs(p) / dot(p, p) - 0.659;
 
        v1 += dot(p, p) * .0015 * (1.8 + sin(length(uv * 13.0) + .5 - iTime * .2));
        v2 += dot(p, p) * .0013 * (1.5 + sin(length(uv * 14.5) + 1.2 - iTime * .3));
        v3 += length(p.xy * 10.) * .0003;
        s += .035;
    }
 
    v1 *= smoothstep(.7, .0, len);
    v2 *= smoothstep(.5, .0, len);
    v3 *= smoothstep(.9, .0, len);
 
    float3 col = float3(v3 * (1.5 + sin(iTime * .2) * .4),
                        (v1 + v3) * .3,
                        v2) + smoothstep(0.2, .0, len) * .85
                          + smoothstep(.0, .6, v3) * .3;
 
    return min(pow(abs(col), 1.2), 1.0);
}

[shader("closesthit")]
void MainUniverseCHS(inout Payload payload, MyAttribute attrib)
{
    if (checkRecursiveLimit(payload))
    {
        return;
    }
    
    // アクセスデータを取得
    uint refHandle = InstanceID();
    BufferRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    MaterialData material = gBufferData[ref.MaterialIndex].Load < MaterialData > (0);
    
     // ---- パラメータ（0 ならフォールバックを使う）----
    float scale = material.scale;
    float bhRadius = material.radius;
    float swirl = material.swirl;
 
    float2 bhPos = material.PlayerPos; // ブラックホールの中心
    float2 uniPos = material.UniversePos; // 宇宙の中心
 
    // ---- 地面上のヒット位置（ワールド空間）----
    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float2 P = worldPos.xz;
 
    // ============================================================
    // 重力レンズは「ワールド空間で光線を曲げる」処理として行う。
    // こうすると曲げた後の位置 lensP を、宇宙とは独立に決められる。
    // ============================================================
    float2 d = P - bhPos;
    float r = length(d); // BHからの距離【ワールド単位】
    float rs = max(r, bhRadius * 0.5f); // 中心で発散させない
    float2 dir = d / max(r, 1e-5f);
 
    float bend = material.strength * bhRadius * bhRadius / rs; // 偏向量 ∝ 1/r
    float2 lensP = P - dir * bend; // 中心へ引き寄せる
 
    // --- 渦（ブラックホール中心まわりに回転）---
    float ang = (bend / bhRadius) * swirl; // 無次元化：r=bhRadius で ang=swirl
    float cs = cos(ang), sn = sin(ang);
    float2 q = lensP - bhPos;
    lensP = bhPos + float2(q.x * cs - q.y * sn,
                           q.x * sn + q.y * cs);
 
    // ============================================================
    // 曲げた後のワールド位置を、宇宙の中心を原点とするUVに変換して評価。
    // → 星雲は uniPos に貼り付いたまま。ブラックホールだけ bhPos で動く。
    // ============================================================
    float2 uv = (lensP - uniPos) * scale;
    float3 col = Universe(uv, material.time);
 
    // --- アインシュタインリング（BH中心基準・ワールド単位）---
    float ring = exp(-pow((r - bhRadius * 1.55f) / (bhRadius * 0.45f), 2.0f));
    col += float3(1.0f, 0.72f, 0.42f) * ring * 0.7f;
 
    // --- 事象の地平線で黒く抜く ---
    col *= smoothstep(bhRadius, bhRadius * 1.12f, r);
 
    // 自発光として出力
    payload.color = saturate(col);
}