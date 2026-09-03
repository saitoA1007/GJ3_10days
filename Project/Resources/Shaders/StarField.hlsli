#ifndef STARFIELD_HLSLI
#define STARFIELD_HLSLI

static const float STAR_PI = 3.141592f;

float StarHash(float3 p)
{
    p = frac(p * float3(0.1031f, 0.11369f, 0.13787f));
    p += dot(p, p.yzx + 19.19f);
    return frac((p.x + p.y) * p.z);
}

float StarLayer(float3 d, float intensity)
{
    return smoothstep(intensity, 0.0f, length(frac(d) - 0.5f))
         * smoothstep(0.98f, 1.0f, StarHash(floor(d)));
}

float3 CalcStarColor(float3 dir, float time, float scale)
{
    float3 c = 0.0f;
    c += StarLayer(dir * (scale + 50.0f), abs(sin(time * 0.5f)) * 0.5f) * float3(0.5f, 0.28f, 0.73f);
    c += StarLayer(dir * scale, abs(cos(time * 0.5f)) * 0.5f) * float3(0.3f, 0.6f, 0.73f);
    c += StarLayer(dir * scale - 50.0f, abs(cos(time)) * 0.5f) * float3(0.5f, 0.58f, 0.43f);
    c += StarLayer(dir * (scale + 50.0f), abs(sin(time)) * 0.5f) * float3(0.2f, 0.2f, 0.8f);
    return c;
}

float3 StarDirFromUV(float2 uv)
{
    float phi = (uv.x - 0.5f) * 2.0f * STAR_PI;
    float y = uv.y * 2.0f - 1.0f;
    float r = sqrt(saturate(1.0f - y * y));
    return float3(r * sin(phi), y, r * cos(phi));
}
#endif