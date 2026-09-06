#include "Common.hlsli"
#include "../LightElement.hlsli"
#include "../StarField.hlsli"

// 宇宙を内包したクリスタル

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

struct CrystalMaterialData
{
    float4x4 uvTransform;       // UVの拡大縮小・回転・オフセット

    float4 baseColor;           // クリスタルの基本色
    float4 rimColor;            // 輪郭の発光色
    float4 dissolveEdgeColor;   // ディゾルブ境界の発光色

    float time;                 // 経過時間
    float dissolveThreshold;    // 0=無傷、1=完全に消える
    float dissolveEdgeWidth;    // ディゾルブ境界のぼかし幅
    float dissolveNoiseScale;   // ディゾルブノイズのタイリング量

    float rimPower;             // 輪郭発光の絞り具合。大きいほど縁だけ光る
    float rimIntensity;         // 輪郭発光の強さ
    float universeIntensity;    // 映り込む宇宙の強さ
    float universeScale;        // 星のスケール

    float ior;                  // 屈折率
    float fresnelStrength;      // 表面反射の効き具合
    uint textureHandle;         // アルベドテクスチャ
    uint dissolveTextureHandle; // ディゾルブ用テクスチャ。0ならプロシージャルノイズ
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

// 宇宙の色を方向から求める。星にうっすらとした星雲を重ねる
float3 SampleUniverse(float3 dir, float time, float scale)
{
    // 星
    float3 col = CalcStarColor(dir, time, scale);

    // 星雲。ゆっくり流れる低周波ノイズで色を作る
    float nebula = FBMNoise(dir * 3.0f + float3(0.0f, 0.0f, time * 0.05f), 4);
    nebula = saturate(nebula * 1.6f - 0.35f);

    float3 nebulaColor = lerp(float3(0.10f, 0.03f, 0.30f), float3(0.35f, 0.12f, 0.55f), nebula);
    col += nebulaColor * nebula;

    return col;
}

// ディゾルブ用のマスクを取得する。値が小さい場所から先に消える
float SampleDissolveMask(CrystalMaterialData material, float3 localPosition, float2 uv)
{
    float mask = FBMNoise(localPosition * max(material.dissolveNoiseScale, 0.001f), 4);
    return saturate(mask * 0.5f + 0.5f);
    
    //return gTexture[material.dissolveTextureHandle].SampleLevel(gSampler, uv, 0).r;
    
    //if (material.dissolveTextureHandle != 0)
    //{
    //    
    //}
    //
    //// テクスチャが無い場合はオブジェクト空間の3Dノイズを使う。UVの継ぎ目が出ない
    //float mask = FBMNoise(localPosition * max(material.dissolveNoiseScale, 0.001f), 4);
    //return saturate(mask * 0.5f + 0.5f);
}

// レイをそのまま貫通させて背後の色を取得する。消えた部分の穴に使う
float3 TraceThrough(float3 worldPosition, int recursive, out float outDepth)
{
    RayDesc throughRay;
    throughRay.Origin = worldPosition;
    throughRay.Direction = WorldRayDirection();
    throughRay.TMin = 0.001f;
    throughRay.TMax = 100000.0f;

    Payload throughPayload;
    throughPayload.color = float3(0.0f, 0.0f, 0.0f);
    throughPayload.recursive = recursive;
    throughPayload.depth = 1.0f;

    TraceRay(
        gRtScene,
        RAY_FLAG_NONE,
        0xFF,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        throughRay,
        throughPayload);

    outDepth = throughPayload.depth;
    return throughPayload.color;
}

[shader("closesthit")]
void MainCrystalCHS(inout Payload payload, MyAttribute attrib)
{
    if (checkRecursiveLimit(payload))
    {
        return;
    }

    // アクセスデータを取得
    uint refHandle = InstanceID();
    BufferRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    CrystalMaterialData material = gBufferData[ref.MaterialIndex].Load<CrystalMaterialData>(0);

    // 頂点データを取得する
    VertexData vtx = GetHitVertex(attrib, ref.vertexHandle, ref.indexHandle);
    // uvをトランスフォーム
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);

    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, ObjectToWorld4x3());
    float3x3 normalMatrix = transpose((float3x3) WorldToObject4x3());
    float3 worldNormal = normalize(mul(vtx.normal, normalMatrix));

    // 深度情報を書き込む
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    payload.depth = clipPos.z / clipPos.w;

    // 視線ベクトル
    float3 rayDir = normalize(WorldRayDirection());
    float3 viewDir = -rayDir;

    // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f)
    {
        worldNormal = -worldNormal;
    }

    // ディゾルブ
    // オブジェクト空間の座標を基準にすることで、模様がモデルに貼り付いたまま動く
    float dissolveMask = SampleDissolveMask(material, vtx.position.xyz, transformedUV.xy);

    float edgeWidth = max(material.dissolveEdgeWidth, 1e-4f);
    // 1で完全に残り、0で完全に消える
    float alpha = smoothstep(material.dissolveThreshold - edgeWidth,
                             material.dissolveThreshold + edgeWidth,
                             dissolveMask);

    // 消えた部分はレイを貫通させ、背後のシーンをそのまま返す
    if (alpha <= 0.001f)
    {
        float throughDepth;
        payload.color = TraceThrough(worldPosition, payload.recursive, throughDepth);
        payload.depth = throughDepth;
        return;
    }

    // 境界付近ほど1に近づく帯。ここを燃えるように光らせる
    float dissolveEdge = 1.0f - saturate(abs(dissolveMask - material.dissolveThreshold) / edgeWidth);
    // ディゾルブが始まっていないときは光らせない
    dissolveEdge *= step(0.001f, material.dissolveThreshold);

    // 宇宙
    float ior = max(material.ior, 1.0f);

    // 内部に閉じ込めた宇宙。屈折方向で見るのでモデルを回すと中の宇宙が動く
    float3 refractDir = refract(rayDir, worldNormal, 1.0f / ior);
    if (dot(refractDir, refractDir) < 1e-6f)
    {
        // 全反射した場合は反射方向で代用する
        refractDir = reflect(rayDir, worldNormal);
    }
    refractDir = normalize(refractDir);
    float3 innerUniverse = SampleUniverse(refractDir, material.time, material.universeScale);

    // 表面に映る宇宙
    float3 reflectDir = normalize(reflect(rayDir, worldNormal));
    float3 surfaceUniverse = SampleUniverse(reflectDir, material.time, material.universeScale);

    // シーンの反射
    float3 sceneReflect = Reflection(worldPosition, worldNormal, payload.recursive);

    // フレネル。輪郭ほど表面の反射が強くなる
    float NdotV = saturate(dot(worldNormal, viewDir));
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;
    float fresnel = saturate(r0 + (1.0f - r0) * pow(1.0f - NdotV, 5.0f));
    fresnel = saturate(fresnel * material.fresnelStrength);

    // テクスチャカラーを取得
    //float4 textureColor = gTexture[material.textureHandle].SampleLevel(gSampler, transformedUV.xy, 0);
    float3 albedoColor = material.baseColor.rgb;

    // 中身は 基本色 + 内部の宇宙、表面は シーンの反射 + 映り込んだ宇宙
    float3 innerColor = albedoColor + innerUniverse * material.universeIntensity;
    float3 surfaceColor = sceneReflect + surfaceUniverse * material.universeIntensity;
    float3 crystalColor = lerp(innerColor, surfaceColor, fresnel);

    // リムライト
    float rimFactor = pow(1.0f - NdotV, max(material.rimPower, 0.001f));
    crystalColor += material.rimColor.rgb * rimFactor * material.rimIntensity;

    // 半分溶けた部分は背後を透かして薄くする
    if (alpha < 0.999f)
    {
        float throughDepth;
        float3 throughColor = TraceThrough(worldPosition, payload.recursive, throughDepth);
        crystalColor = lerp(throughColor, crystalColor, alpha);
    }

    // 境界の発光は透過に影響されず常に加算する
    crystalColor += material.dissolveEdgeColor.rgb * dissolveEdge;

    payload.color = crystalColor;
}
