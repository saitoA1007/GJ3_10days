#include "Common.hlsli"

// ブラックホールの重力レンズのマテリアル
struct MaterialData
{
    float radius;        // 事象の地平線の半径
    float strength;      // 光を曲げる強さ
    float swirl;         // 降着円盤のような渦の強さ
    float glowIntensity; // ふちの発光強度

    float3 glowColor;    // ふちの発光色
    float glowWidth;     // 発光が広がる範囲
};

[shader("closesthit")]
void MainBlackHoleLensCHS(inout Payload payload, MyAttribute attrib)
{
    if (checkRecursiveLimit(payload))
    {
        return;
    }

    // アクセスデータを取得
    uint refHandle = InstanceID();
    BufferRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    MaterialData material = gBufferData[ref.MaterialIndex].Load<MaterialData>(0);

    float bhRadius = material.radius;

    float3 bhCenter = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), ObjectToWorld4x3());

    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 rayDir = normalize(WorldRayDirection());

    // レイ上で中心に最も近づく点との距離を求める
    float3 toCenter = bhCenter - worldPos;
    float tClosest = dot(toCenter, rayDir);
    float3 closestPoint = worldPos + rayDir * tClosest;

    float3 offset = bhCenter - closestPoint;
    float b = length(offset); // 衝突径数
    float3 pullDir = offset / max(b, 1e-5f); // 中心へ引き寄せる方向

    // 事象の地平線の内側は完全に黒
    if (b < bhRadius)
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    // 偏向量
    float bs = max(b, bhRadius * 0.5f);
    float bend = material.strength * bhRadius * bhRadius / bs;

    // 光を中心方向へ曲げる
    float3 bentDir = normalize(rayDir + pullDir * bend);

    // 降着円盤のような渦。視線方向を軸にレイを回転させる
    if (abs(material.swirl) > 0.0001f)
    {
        float ang = bend * material.swirl;
        float cs = cos(ang), sn = sin(ang);
        float3 axis = rayDir;
        // ロドリゲスの回転公式
        bentDir = bentDir * cs + cross(axis, bentDir) * sn + axis * dot(axis, bentDir) * (1.0f - cs);
        bentDir = normalize(bentDir);
    }

    // 曲げた方向へレイを飛ばし、実際に背後にあるものを歪めて映す
    RayDesc rayDesc;
    rayDesc.Origin = worldPos;
    rayDesc.Direction = bentDir;
    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 100000.0f;

    Payload lensPayload;
    lensPayload.color = float3(0.0f, 0.0f, 0.0f);
    lensPayload.recursive = payload.recursive;
    lensPayload.depth = 0.0f;

    TraceRay(
        gRtScene,
        RAY_FLAG_NONE,
        0xFF,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        lensPayload);

    // 事象の地平線に近いほど暗く落とし込む
    float darken = smoothstep(bhRadius, bhRadius * 1.5f, b);

    // ふちのリング状の発光
    float edgeDist = (b - bhRadius) / max(bhRadius * material.glowWidth, 1e-4f);
    float glow = exp(-edgeDist * edgeDist);
    float3 glowColor = material.glowColor * glow * material.glowIntensity;

    payload.color = lensPayload.color * darken + glowColor;
}
