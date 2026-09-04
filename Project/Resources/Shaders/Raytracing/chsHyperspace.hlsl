#include "Common.hlsli"
#include "../LightElement.hlsli"
#include "../StarField.hlsli"

// ハイパースペース演出のフェーズ
#define HYPERSPACE_PHASE_IDLE   0
#define HYPERSPACE_PHASE_JUMP   1 // 突入

struct MaterialData
{
    float time; // 星アニメーション
    float phaseTime; // 現在のフェーズが始まってからの経過時間
    uint phase; // HYPERSPACE_PHASE_
    float pad;
};

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
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

// ================================================================
// 突入バースト演出
// Adapted from: https://www.shadertoy.com/view/MlKBWw
// ================================================================
static const float HJ_NUM_SLICES = 125.0f;
static const float HJ_MAX_SLICE_OFFSET = 0.4f;
static const float HJ_T_JUMP = 0.75f;
static const float HJ_JUMP_SPEED = 15.0f;
static const float3 HJ_BLUE_COL = float3(0.3f, 0.3f, 0.5f);
static const float3 HJ_WHITE_COL = float3(0.85f, 0.85f, 0.9f);
static const float3 HJ_FLARE_COL = float3(0.9f, 0.9f, 1.4f);

float HJ_SdLine(float2 p, float2 a, float2 b, float ring)
{
    float2 pa = p - a, ba = b - a;
    float h = saturate(dot(pa, ba) / dot(ba, ba));
    return length(pa - ba * h) - ring;
}

float HJ_Rand(float2 co)
{
    return frac(sin(dot(co, float2(12.9898f, 78.233f))) * 43758.5453f);
}

// Lens flare from: https://www.shadertoy.com/view/XdfXRX と https://www.shadertoy.com/view/4sX3Rs
float3 HJ_LensFlare(float3 uv, float3 pos, float flareSize, float angOffset)
{
    float z = uv.z / length(uv.xy);
    float2 mainVec = uv.xy - pos.xy;
    float dist = length(mainVec);
    float numPoints = 2.71f;
    float diskSize = 0.2f;
    float invSize = 1.0f / flareSize;
    float ang = atan2(mainVec.y, mainVec.x) + angOffset;
    float fade = (z < 0.0f) ? -z : 1.0f;

    float f0 = 1.0f / (dist * invSize + 1.0f);
    f0 = f0 + f0 * (0.1f * sin((sin(ang * 2.0f + pos.x) * 4.0f - cos(ang * 3.0f + pos.y)) * numPoints) + diskSize);
    if (z < 0.0f)
        return saturate(lerp(float3(f0, f0, f0), float3(0.0f, 0.0f, 0.0f), 0.75f * fade));
    else
        return float3(f0, f0, f0);
}

float3 HJ_ColorGrade(float3 color, float factor, float factor2)
{
    float w = color.x + color.y + color.z;
    return lerp(color, float3(w, w, w) * factor, w * factor2);
}

float3 CalcHyperspaceJumpColor(float2 p, float t)
{
    float3 color = float3(0.0f, 0.0f, 0.0f);
    float3 v = float3(p, 1.0f);

    // 全体をフェードイン
    float fade = clamp(lerp(0.1f, 1.1f, t * 2.0f), 0.0f, 2.0f);

    // 1レイヤーにつき1本のトレイルを描画。レイヤーを重ねて密度を上げる
    [loop]
    for (float i = 0.0f; i < 24.0f; i += 1.0f)
    {
        float angle = atan2(v.y, v.x) / 3.141592f / 2.0f + 0.13f * i;

        float slice = floor(angle * HJ_NUM_SLICES);
        float sliceFract = frac(angle * HJ_NUM_SLICES);

        // トレイルをスライス中央からランダムにずらす
        float sliceOffset = HJ_MAX_SLICE_OFFSET * HJ_Rand(float2(slice, 4.0f + i * 25.0f)) - (HJ_MAX_SLICE_OFFSET * 0.5f);

        float dist = 10.0f * HJ_Rand(float2(slice, 1.0f + i * 10.0f)) - 5.0f;
        float z = dist * v.z / length(v.xy);
        float f = sign(dist);
        if (f == 0.0f)
            f = 1.0f;
        float fSpeed = f * (0.1f * HJ_Rand(float2(slice, 1.0f + i * 10.0f)) + i * 0.01f);
        float fJumpSpeed = f * HJ_JUMP_SPEED;

        float trailStart = 10.0f * HJ_Rand(float2(slice, 0.0f + i * 10.0f)) - 5.0f;
        // ジャンプ終盤でトレイルの起点を加速させる
        trailStart -= lerp(0.0f, fJumpSpeed, smoothstep(HJ_T_JUMP, 1.0f, t));
        float trailEnd = trailStart - t * fSpeed;

        float trailX = smoothstep(trailStart, trailEnd, z);
        float3 trailColor = lerp(HJ_BLUE_COL, HJ_WHITE_COL, trailX);

        // スライス中心の理想的なトレイルまでの距離
        float h = HJ_SdLine(
            float2(sliceFract + sliceOffset, z),
            float2(0.5f, trailStart),
            float2(0.5f, trailEnd),
            lerp(0.0f, 0.015f, t * z));

        float threshold = 0.09f;
        h = (h < 0.01f) ? 1.0f : 0.85f * smoothstep(threshold, 0.0f, abs(h));

        trailColor *= fade * h;
        color = max(color, trailColor);
    }

    // 中心のフレア
    float flareSize = lerp(0.0f, 0.1f, smoothstep(0.35f, HJ_T_JUMP + 0.2f, t));
    flareSize += lerp(0.0f, 20.0f, smoothstep(HJ_T_JUMP + 0.05f, 1.0f, t));
    float3 flare = HJ_FLARE_COL * HJ_LensFlare(v, float3(0.0f, 0.0f, 0.0f), flareSize, t);
    color += HJ_ColorGrade(flare, 0.5f, 0.1f);

    // ホワイトアウト
    color += lerp(0.0f, 1.0f, smoothstep(HJ_T_JUMP + 0.1f, 1.0f, t));

    return color;
}

[shader("closesthit")]
void MainHyperspaceCHS(inout Payload payload, MyAttribute attrib)
{
    if (checkRecursiveLimit(payload))
    {
        return;
    }

    uint refHandle = InstanceID();
    BufferRef ref = gBufferRefs[refHandle];
    MaterialData material = gBufferData[ref.MaterialIndex].Load < MaterialData > (0);
    
      // 頂点データを取得する
    VertexData vtx = GetHitVertex(attrib, ref.vertexHandle, ref.indexHandle);
    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, ObjectToWorld4x3());
    
    // 深度情報を書き込む
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    payload.depth = clipPos.z / clipPos.w;

    float2 fragCoord = float2(DispatchRaysIndex().xy) + 0.5f;
    float2 resolution = float2(DispatchRaysDimensions().xy);
    float2 p = (2.0f * fragCoord - resolution) / min(resolution.x, resolution.y);

    float3 col = float3(0.0f, 0.0f, 0.0f);

    if (material.phase == HYPERSPACE_PHASE_JUMP)
    {
        // 突入
        float t = saturate(material.phaseTime);
        col = CalcHyperspaceJumpColor(p, t);
    }
    
     // 星空
    float3 starDir = normalize(WorldRayDirection());
    col += CalcStarColor(starDir, material.time, 400.0f);

    payload.color = saturate(col);
}