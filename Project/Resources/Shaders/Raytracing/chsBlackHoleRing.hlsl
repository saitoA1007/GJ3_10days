#include "Common.hlsli"

// ブラックホールの周りのリング

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

struct MaterialData
{
    float4x4 uvTransform;    // UVの拡大縮小・回転・オフセット

    float innerRadius;       // リング内側の半径
    float outerRadius;       // リング外側の半径
    float time;              // 経過時間
    float scrollSpeed;       // UVスクロールの速さ

    float noiseScale;        // ノイズのタイリング量
    float noiseJitter;       // ノイズの粒のばらつき具合
    float driftSpeed;        // ノイズが時間で揺らぐ速さ
    float dissolveThreshold; // ディゾルブの閾値（0〜1）

    float dissolveEdge;      // ディゾルブ境界のぼかし幅
    float densityPower;      // 中心に近づくほど密度・発光を強める指数
    float emissionIntensity; // 発光の基準強度
    float pad0;

    float3 emissionColor;    // 発光色
    float pad1;
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
        uint index = gBufferData[indexHandle].Load<uint>((start + i) * 4);

        VertexData v = gBufferData[vertexHandle].Load<VertexData>(index * VERTEX_STRIDE);

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

// タイル状に散らばる円形ブロブ1個分の濃度を手続き的に求める
float RingBlobNoise(float3 p, float jitter)
{
    float3 cellID;
    float minDist, secondDist;
    Voronoi3D(p, jitter, cellID, minDist, secondDist);

    // セルごとに半径をランダム化する
    float radius = lerp(0.2f, 0.5f, Hash1D(cellID + 11.3f));

    // 中心が明るく外側へ滑らかに減衰する円
    float blob = 1.0f - smoothstep(0.0f, radius, minDist);

    // セル境界に近いほど寄与をフェードさせる。
    float edgeFade = smoothstep(0.0f, max(jitter, 0.05f) * 0.5f, secondDist - minDist);

    return blob * edgeFade;
}

// 複数スケールのブロブを重ねて、有機的で密度感のあるノイズにする
float RingNoise(float2 uv, float z, float jitter)
{
    float n = RingBlobNoise(float3(uv, z), jitter) * 0.6f;
    n += RingBlobNoise(float3(uv * 2.17f + 5.2f, z * 1.3f - 2.0f), jitter) * 0.3f;
    n += RingBlobNoise(float3(uv * 4.31f - 3.7f, z * 1.7f + 4.0f), jitter) * 0.1f;
    return saturate(n);
}

[shader("closesthit")]
void MainBlackHoleRingCHS(inout Payload payload, MyAttribute attrib)
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

    VertexData vtx = GetHitVertex(attrib, ref.vertexHandle, ref.indexHandle);

    // ローカル空間での中心からの距離
    float localRadius = length(vtx.position.xz);
    float radiusT = saturate((localRadius - material.innerRadius) / max(material.outerRadius - material.innerRadius, 1e-4f));

    // 中心に近いほど 1 に近づく係数
    float coreFactor = pow(1.0f - radiusT, max(material.densityPower, 0.001f));

    // UVをスクロールさせてリングが回転しているように見せる
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);
    float2 scrollOffset = float2(material.time * material.scrollSpeed, 0.0f);
    float2 noiseUV = transformedUV.xy * material.noiseScale + scrollOffset;

    // 手続き的に生成したブロブノイズをディゾルブのマスクとして使用
    float noise = RingNoise(noiseUV, material.time * material.driftSpeed, material.noiseJitter);

    // 中心に近いほど閾値を下げる＝ノイズが消えにくくなり密度が濃く見える
    float threshold = saturate(material.dissolveThreshold - coreFactor);
    float density = smoothstep(threshold - material.dissolveEdge, threshold + material.dissolveEdge, noise);

    // 背後のシーンを通し、半透明な円盤として振る舞う
    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    RayDesc throughRay;
    throughRay.Origin = worldPos;
    throughRay.Direction = WorldRayDirection();
    throughRay.TMin = 0.001f;
    throughRay.TMax = 100000.0f;

    Payload throughPayload;
    throughPayload.color = float3(0.0f, 0.0f, 0.0f);
    throughPayload.recursive = payload.recursive;
    throughPayload.depth = 0.0f;

    TraceRay(
        gRtScene,
        RAY_FLAG_NONE,
        0xFF,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        throughRay,
        throughPayload);

    // 中心に近いほど発光を強めながら、密度でマスクして加算する
    float3 glow = material.emissionColor * material.emissionIntensity * (1.0f + coreFactor * material.densityPower);

    payload.color = throughPayload.color + glow * density;
}
