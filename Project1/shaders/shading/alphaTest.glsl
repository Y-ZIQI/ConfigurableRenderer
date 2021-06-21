//#include "Utils/Math/MathConstants.slangh"
//#include "Scene/Material/MaterialDefines.slangh"

//__exported import Utils.Color.ColorHelpers;

#define M_PI                3.14159265358979323846  // pi
#define M_2PI               6.28318530717958647693  // 2pi

#define _DEFAULT_ALPHA_TEST

/*******************************************************************
    Spherical map sampling
*******************************************************************/
vec2 dirToSphericalCrd(vec3 direction)
{
    vec3 p = normalize(direction);
    vec2 uv;
    uv.x = (1 + atan(-p.z, p.x) / M_PI) * 0.5;
    uv.y = acos(p.y) / M_PI;
    return uv;
}

vec3 sphericalCrdToDir(vec2 uv)
{
    float phi = M_PI * uv.y;
    float theta = M_2PI * uv.x - (M_PI / 2.0f);

    vec3 dir;
    dir.x = sin(phi) * sin(theta);
    dir.y = cos(phi);
    dir.z = sin(phi) * cos(theta);

    return normalize(dir);
}

/*******************************************************************
    Sample Patterns
*******************************************************************/

float radicalInverse(uint i)
{
    i = (i & 0x55555555) << 1 | (i & 0xAAAAAAAA) >> 1;
    i = (i & 0x33333333) << 2 | (i & 0xCCCCCCCC) >> 2;
    i = (i & 0x0F0F0F0F) << 4 | (i & 0xF0F0F0F0) >> 4;
    i = (i & 0x00FF00FF) << 8 | (i & 0xFF00FF00) >> 8;
    i = (i << 16) | (i >> 16);
    return float(i) * 2.3283064365386963e-10;
}

vec2 getHammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), radicalInverse(i));
}

/*******************************************************************
    Ray tracing
*******************************************************************/

/** Computes new ray origin based on hit position to avoid self-intersections.
    The function assumes that the hit position has been computed by barycentric
    interpolation, and not from the ray t which is less accurate.

    The method is described in Ray Tracing Gems, Chapter 6, "A Fast and Robust
    Method for Avoiding Self-Intersection" by Carsten Wächter and Nikolaus Binder.

    \param[in] pos Ray hit position.
    \param[in] normal Face normal of hit surface (normalized). The offset will be in the positive direction.
    \return Ray origin of the new ray.
*/
/*vec3 computeRayOrigin(vec3 pos, vec3 normal)
{
    const float origin = 1.0 / 32.0;
    const float fScale = 1.0 / 65536.0;
    const float iScale = 256.0;

    // Per-component integer offset to bit representation of fp32 position.
    ivec3 iOff = ivec3(normal * iScale);
    vec3 iPos = vec3(ivec3(pos) + ((pos < 0.0) ? -iOff : iOff));

    // Select per-component between small fixed offset or above variable offset depending on distance to origin.
    vec3 fOff = normal * fScale;
    return (abs(pos) < origin) ? pos + fOff : iPos;
}*/

/** Ray-sphere intersection.
    This function implements the standard analytic test with improvements to floating-point precision
    and returns the closest hit.
    \param[in] rayOrigin Ray origin.
    \param[in] rayDir Ray direction (does not have to be normalized).
    \param[in] center Sphere center.
    \param[in] radius Sphere radius.
    \param[in] intersectionPos Position on the sphere for the closest intersection (if any).
    \return True if the ray intersects the sphere.
*/
/*bool intersectRaySphere(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius, out vec3 intersectionPos)
{
    // Implementation is taken from Chapter 7 of Ray-Tracing Gems
    vec3 f = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = dot(-f, rayDir);
    float discriminant = sphereRadius * sphereRadius - dot(f + b / a * rayDir, f + b / a * rayDir);

    // Negative discriminant means ray missed sphere.
    if (discriminant < 0.0) return false;

    // If b and discriminant are both 0, then the ray's origin lies on the sphere
    if (b == 0 && discriminant == 0)
    {
        intersectionPos = rayOrigin;
        return true;
    }

    // There are two solutions t0 and t1, but one or both may be negative.
    float c = dot(f, f) - sphereRadius * sphereRadius;
    float signB = (b < 0) ? -1 : 1;
    float q = b + signB * sqrt(a * discriminant);
    float t0 = c / q;
    float t1 = q / a;

    float tc = t0 < 0.f ? t1 : t0; // tc is the closest hit we care about
    if (tc < 0.f) return false;

    intersectionPos = rayOrigin + tc * rayDir;
    return true;
}*/

/*******************************************************************
                    Shading
*******************************************************************/
/*
vec4 applyAmbientOcclusion(vec4 color, Texture2D aoTex, SamplerState s, vec2 texC)
{
    float aoFactor = aoTex.SampleLevel(s, texC, 0).r;
    return vec4(color.rgb * aoFactor, color.a);
}

// TODO: this function is broken an may return negative values.
float getMetallic(vec3 diffuse, vec3 spec)
{
    // This is based on the way that UE4 and Substance Painter 2 converts base+metallic+specular level to diffuse/spec colors
    // We don't have the specular level information, so the assumption is that it is equal to 0.5 (based on the UE4 documentation)
    // Note that I'm using the luminance here instead of the actual colors. The reason is that there's no guaraentee that all RGB channels will end up with the same metallic value
    float d = luminance(diffuse);
    float s = luminance(spec);
    if (s == 0) return 0;
    float a = 0.04;
    float b = s + d - 0.08;
    float c = 0.04 - s;
    float root = sqrt(b*b - 0.16*c);
    float m = (root - b) * 12.5;
    return m;
}
*/
/*******************************************************************
                    Motion vectors
*******************************************************************/

/** Calculate screen-space motion vector.
    \param[in] pixelCrd Sample in current frame expressed in pixel coordinates with origin in the top-left corner.
    \param[in] prevPosH Sample in previous frame expressed in homogeneous clip space coordinates. Note that the definition differs between D3D12 and Vulkan.
    \param[in] renderTargetDim Render target dimension in pixels.
    \return Motion vector pointing from current to previous position expressed in sceen space [0,1] with origin in the top-left corner.
*/
/*
vec2 calcMotionVector(vec2 pixelCrd, vec4 prevPosH, vec2 renderTargetDim)
{
    vec2 prevCrd = prevPosH.xy / prevPosH.w;
#ifdef FALCOR_VK
    prevCrd *= vec2(0.5, 0.5);
#else
    prevCrd *= vec2(0.5, -0.5);
#endif
    prevCrd += 0.5f;
    vec2 normalizedCrd = pixelCrd / renderTargetDim;
    return prevCrd - normalizedCrd;
}
*/
/*******************************************************************
                    Alpha test
*******************************************************************/
// Evaluate alpha test and return true if point should be discarded
bool evalBasicAlphaTest(float alpha, float threshold)
{
    return alpha < threshold;
}
/*******************************************************************
                    Hashed Alpha Test
*******************************************************************/
// Evaluate alpha test and return true if point should be discarded
bool evalHashedAlphaTest(float alpha, float materialThreshold, float hashedThreshold)
{
    float compareTo = hashedThreshold <= 0 ? materialThreshold : clamp(hashedThreshold, 0.0, 1.0);
    return alpha < compareTo;
}

float sineHash(vec2 p)
{
    return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x))));
}

float sineHash3D(vec3 p)
{
    return sineHash(vec2(sineHash(p.xy), p.z));
}

float calculateHashedAlpha(vec3 hashInputCoord, float hashScale, bool useAnisotropy)
{
    // Provide a decent default to our alpha threshold
    float alphaCompare = 0.5f;

    if (useAnisotropy)
    {
        //////  Anisotropic version

        // Find the discretized derivatives of our coordinates
        vec3 anisoDeriv = max(abs(dFdx(hashInputCoord)), abs(dFdy(hashInputCoord)));
        vec3 anisoScales = vec3(0.707 / (hashScale * anisoDeriv.x),
                                    0.707 / (hashScale * anisoDeriv.y),
                                    0.707 / (hashScale * anisoDeriv.z));
        // Find log-discretized noise scales
        vec3 scaleFlr = vec3(exp2(floor(log2(anisoScales.x))),
                                 exp2(floor(log2(anisoScales.y))),
                                 exp2(floor(log2(anisoScales.z))));
        vec3 scaleCeil = vec3(exp2(ceil(log2(anisoScales.x))),
                                  exp2(ceil(log2(anisoScales.y))),
                                  exp2(ceil(log2(anisoScales.z))));
        // Compute alpha thresholds at our two noise scales
        vec2 alpha = vec2(sineHash3D(floor(scaleFlr * hashInputCoord)),
                              sineHash3D(floor(scaleCeil * hashInputCoord)));
        // Factor to linearly interpolate with
        vec3 fractLoc = vec3(fract(log2(anisoScales.x)),
                                 fract(log2(anisoScales.y)),
                                 fract(log2(anisoScales.z)));
        vec2 toCorners = vec2(length(fractLoc),
                                  length(vec3(1.0, 1.0, 1.0) - fractLoc));
        float lerpFactor = toCorners.x / (toCorners.x + toCorners.y);
        // Interpolate alpha threshold from noise at two scales
        float x = (1 - lerpFactor) * alpha.x + lerpFactor * alpha.y;
        // Pass into CDF to compute uniformly distrib threshold
        float a = min(lerpFactor, 1 - lerpFactor);
        vec3 cases = vec3(x * x / (2 * a * (1 - a)), (x - 0.5 * a) / (1 - a), 1.0 - ((1 - x) * (1 - x) / (2 * a * (1 - a))));
        // Find our final, uniformly distributed alpha threshold
        alphaCompare = (x < (1 - a)) ? ((x < a) ? cases.x : cases.y) : cases.z;
        alphaCompare = clamp(alphaCompare, 1.0e-6, 1.0);
    }
    else
    {
        //////  Isotropic version

        // Find the discretized derivatives of our coordinates
        float maxDeriv = max(length(dFdx(hashInputCoord)), length(dFdy(hashInputCoord)));
        float pixScale = 1.0 / (hashScale * maxDeriv);
        // Find two nearest log-discretized noise scales
        vec2 pixScales = vec2(exp2(floor(log2(pixScale))), exp2(ceil(log2(pixScale))));
        // Compute alpha thresholds at our two noise scales
        vec2 alpha = vec2(sineHash3D(floor(pixScales.x * hashInputCoord)), sineHash3D(floor(pixScales.y * hashInputCoord)));
        // Factor to interpolate lerp with
        float lerpFactor = fract(log2(pixScale));
        // Interpolate alpha threshold from noise at two scales
        float x = (1 - lerpFactor) * alpha.x + lerpFactor * alpha.y;
        float a = min(lerpFactor, 1 - lerpFactor);
        // Pass into CDF to compute uniformly distrib threshold
        vec3 cases = vec3(x * x / (2 * a * (1 - a)), (x - 0.5 * a) / (1 - a), 1.0 - ((1 - x) * (1 - x) / (2 * a * (1 - a))));
        // Find our final, uniformly distributed alpha threshold
        alphaCompare = (x < (1 - a)) ? ((x < a) ? cases.x : cases.y) : cases.z;
        alphaCompare = clamp(alphaCompare, 1.0e-6, 1.0);
    }

    return alphaCompare;
}

/*******************************************************************
    Alpha test
*******************************************************************/

/** Evaluates alpha test, returning true if pixel should be discarded.
    \todo calculateHashedAlpha requires dFdx/dFdy, so cannot be used in RT mode.
*/
bool evalAlphaTest(float alpha, float threshold, vec3 posW)
{
    float hashedAlphaScale = 1.0;
#ifdef _HASHED_ALPHA_SCALE
    hashedAlphaScale = _HASHED_ALPHA_SCALE;
#endif

    /* Evaluate alpha test material modifier */
#ifdef _DEFAULT_ALPHA_TEST
    return evalBasicAlphaTest(alpha, threshold);
#elif defined(_HASHED_ALPHA_TEST_ANISOTROPIC)
    float hashedThreshold = calculateHashedAlpha(posW, hashedAlphaScale, true);
    return evalHashedAlphaTest(alpha, threshold, hashedThreshold);
#else
    // Default to isotropic hashed alpha test
    float hashedThreshold = calculateHashedAlpha(posW, hashedAlphaScale, false);
    return evalHashedAlphaTest(alpha, threshold, hashedThreshold);
#endif
}

/*******************************************************************
   	Random numbers based on Mersenne Twister
*******************************************************************/
uint rand_init(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0;
    uint v1 = val1;
    uint s0 = 0;

    for(uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1<<4)+0xa341316c)^(v1+s0)^((v1>>5)+0xc8013ea4);
        v1 += ((v0<<4)+0xad90777d)^(v0+s0)^((v0>>5)+0x7e95761e);
    }

    return v0;
}

float rand_next(inout uint s)
{
    uint LCG_A = 1664525;
    uint LCG_C = 1013904223;
    s = (LCG_A * s + LCG_C);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}
