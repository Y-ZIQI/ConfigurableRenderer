//++`shaders/shading/defines.glsl`
//++`shaders/shading/ShadingData.glsl`

#define SmithGGXFalcor  0       // Frostbite
#define SmithGGXUE4    1
#define SmithGGX SmithGGXFalcor

#define DiffuseBrdfLambert      0
#define DiffuseBrdfDisney       1
#define DiffuseBrdfFrostbite    2

#ifndef DiffuseBrdf
#define DiffuseBrdf DiffuseBrdfLambert
#endif

#define SpecularMaskingFunctionSmithGGXSeparable    0       ///< Used by UE4.
#define SpecularMaskingFunctionSmithGGXCorrelated   1       ///< Used by Frostbite. This is the more accurate form (default).

#ifndef SpecularMaskingFunction
#define SpecularMaskingFunction SpecularMaskingFunctionSmithGGXCorrelated
#endif

vec3 fresnelSchlick(vec3 f0, vec3 f90, float u)
{
    return f0 + (f90 - f0) * pow(1 - u, 5);
}

/** Disney's diffuse term. Based on https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
*/
float disneyDiffuseFresnel(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float fd90 = 0.5 + 2 * LdotH * LdotH * linearRoughness;
    float fd0 = 1.0;
    float lightScatter = fresnelSchlick(vec3(fd0), vec3(fd90), NdotL).r;
    float viewScatter = fresnelSchlick(vec3(fd0), vec3(fd90), NdotV).r;
    return lightScatter * viewScatter;
}

vec3 evalDiffuseDisneyBrdf(ShadingData sd, LightSample ls)
{
    return disneyDiffuseFresnel(sd.NdotV, ls.NdotL, ls.LdotH, sd.linearRoughness) * M_1_PI * sd.diffuse.rgb;
}

/** Lambertian diffuse
*/
vec3 evalDiffuseLambertBrdf(ShadingData sd, LightSample ls)
{
    return sd.diffuse.rgb * M_1_PI;
}

/** Frostbites's diffuse term. Based on https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
*/
vec3 evalDiffuseFrostbiteBrdf(ShadingData sd, LightSample ls)
{
    float energyBias = mix(0.0, 0.5, sd.linearRoughness);
    float energyFactor = mix(1.0, 1.0 / 1.51, sd.linearRoughness);

    float fd90 = energyBias + 2 * ls.LdotH * ls.LdotH * sd.linearRoughness;
    float fd0 = 1;
    float lightScatter = fresnelSchlick(vec3(fd0), vec3(fd90), ls.NdotL).r;
    float viewScatter = fresnelSchlick(vec3(fd0), vec3(fd90), sd.NdotV).r;
    return (viewScatter * lightScatter * energyFactor * M_1_PI) * sd.diffuse.rgb;
}

vec3 evalSpecularIBL(ShadingData sd){
    //return fresnelSchlick(vec3(1.0), max(vec3(1.0 - sd.linearRoughness), vec3(1.0)), max(sd.NdotV, 0.0));
    return fresnelSchlick(sd.specular, max(vec3(1.0 - sd.linearRoughness), sd.specular), max(sd.NdotV, 0.0));
}

vec3 evalDiffuseBrdf(ShadingData sd, LightSample ls)
{
#if DiffuseBrdf == DiffuseBrdfLambert
    return evalDiffuseLambertBrdf(sd, ls);
#elif DiffuseBrdf == DiffuseBrdfDisney
    return evalDiffuseDisneyBrdf(sd, ls);
#elif DiffuseBrdf == DiffuseBrdfFrostbite
    return evalDiffuseFrostbiteBrdf(sd, ls);
#endif
}

float evalGGX(float ggxAlpha, float NdotH)
{
    float a2 = ggxAlpha * ggxAlpha;
    float d = ((NdotH * a2 - NdotH) * NdotH + 1);
    return a2 / (d * d);
}

float evalSmithGGX(float NdotL, float NdotV, float ggxAlpha)
{
    NdotV = max(0.0, NdotV);
#if SmithGGX == SmithGGXFalcor
    // Optimized version of Smith, already taking into account the division by (4 * NdotV)
    float a2 = ggxAlpha * ggxAlpha;
    // `NdotV *` and `NdotL *` are inversed. It's not a mistake.
    float ggxv = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    float ggxl = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    return 0.5 / (ggxv + ggxl);
#else //if SmithGGX == SmithGGXUE4
    //float ggxv = NdotV * (2.0 - a2) + a2;
    //float ggxl = NdotL * (2.0 - a2) + a2;
    //return 4.0 / ggxl * ggxv;
    float ggxv = NdotL * ((1.0 - ggxAlpha) * NdotV + ggxAlpha);
    float ggxl = NdotV * ((1.0 - ggxAlpha) * NdotL + ggxAlpha);
    return 0.5 / (ggxl + ggxv);
#endif
}

vec3 evalSpecularBrdf(ShadingData sd, LightSample ls)
{
    float ggxAlpha = sd.ggxAlpha;

    float D = evalGGX(ggxAlpha, ls.NdotH);
    float G = evalSmithGGX(ls.NdotL, sd.NdotV, ggxAlpha);
    vec3 F = fresnelSchlick(sd.specular, vec3(1.0), clamp(ls.LdotH, 0.0, 1.0));
    return D * G * F * M_1_PI;
}
