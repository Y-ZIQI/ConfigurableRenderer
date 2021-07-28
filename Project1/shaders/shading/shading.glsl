//++`shaders/shading/BRDF.glsl`

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

struct DirectionalLight{
    vec3 intensity;
    vec3 direction;
    float ambient;

    bool has_shadow;
    mat4 viewProj;
    sampler2D shadowMap;
    float resolution;
    float light_size;
};

struct PointLight{
    vec3 intensity;
    vec3 position;
    vec3 direction;
    float ambient;

    float range;
    float opening_angle, cos_opening_angle;
    float penumbra_angle;

    float constant;
    float linear;
    float quadratic;

    bool has_shadow;
    mat4 viewProj;
    sampler2D shadowMap;
    float resolution;
    float light_size;
};

#ifndef MAX_DIRECTIONAL_LIGHT
#define MAX_DIRECTIONAL_LIGHT 2
#endif
#ifndef MAX_POINT_LIGHT
#define MAX_POINT_LIGHT 15
#endif

uniform int dirLtCount;
uniform int ptLtCount;
uniform DirectionalLight dirLights[MAX_DIRECTIONAL_LIGHT];
uniform PointLight ptLights[MAX_POINT_LIGHT];

//#define EVAL_IBL
#ifndef MAX_IBL_LIGHT
#define MAX_IBL_LIGHT 8
#endif
//#define IBL_GAMMACORRECTION
uniform int iblCount;
uniform sampler2D brdfLUT;
struct IBL{
    vec3 intensity;
    vec3 position;
    float range;
    float miplevel;
    samplerCube prefilterMap;
};
uniform IBL ibls[MAX_IBL_LIGHT];
float center_distance[MAX_IBL_LIGHT];

/********************************************Config********************************************/
// Shadow map related variables
//#define SHADOW_HARD
//#define SHADOW_SOFT_ESM
//#define SHADOW_SOFT_PCF
//#define SHADOW_SOFT_PCSS
//#define SHADOW_SOFT_EVSM
#define SM_ESM_LOD 2
#define SM_PCF_FILTER 0.002
#define NUM_SAMPLES 40

#define EPS 1e-5
#define MAX_BIAS 2.0
#define MIN_BIAS 0.2

#define EVAL_DIFFUSE
#define EVAL_SPECULAR

#ifndef SHADING_MODEL
#define SHADING_MODEL 1
#endif
/***************************************Shadow Making******************************************/
bool posFromLight(mat4 viewProj, vec3 position, out vec3 ndc){
    vec4 fromLight = viewProj * vec4(position, 1.0);
    ndc = fromLight.xyz / fromLight.w;
    vec3 abs_ndc = abs(ndc);
    if(max(max(abs_ndc.x, abs_ndc.y), abs_ndc.z) >= 1.0)
        return false;
    ndc = ndc * 0.5 + 0.5;
    return true;
}
const float posi_c = 40.0;
const float nega_c = 5.0;
float EVSM_dir_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;

    float expz = exp(ndc.z * posi_c), _expz = -exp(-ndc.z * nega_c);
    ATOMIC_COUNT_INCREMENT
    vec4 e = texture(dirLights[index].shadowMap, ndc.xy);
    if(expz <= e.r * exp(bias * posi_c)) return 1.0;
    float e2 = e.g - e.r * e.r, _e2 = e.a - e.b * e.b;
    float max1 = e2 / (e2 + (expz - e.r) * (expz - e.r));
    float max2 = _e2 / (_e2 + (_expz - e.b) * (_expz - e.b));
    return min(max1, max2);
}
float EVSM_pt_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;

    float expz = exp(ndc.z * posi_c), _expz = -exp(-ndc.z * nega_c);
    ATOMIC_COUNT_INCREMENT
    vec4 e = texture(ptLights[index].shadowMap, ndc.xy);
    if(expz <= e.r * exp(bias * posi_c)) return 1.0;
    float e2 = e.g - e.r * e.r, _e2 = e.a - e.b * e.b;
    float max1 = e2 / (e2 + (expz - e.r) * (expz - e.r));
    float max2 = _e2 / (_e2 + (_expz - e.b) * (_expz - e.b));
    return min(max1, max2);
}

uniform vec2 samples[NUM_SAMPLES];
mat2 rotateM;

highp float rand_2to1(vec2 uv ) {
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, M_PI );
	return fract(sin(sn) * c);
}
void setRandomRotate(vec2 uv){
    float angle = rand_2to1(uv) * M_2PI;
    rotateM = mat2(
        cos(angle), sin(angle),
        -sin(angle), cos(angle)
    );
}
float findBlocker(sampler2D sm, vec2 uv, float zReceiver, float searchRadius){
    float depthSum = 0.0;
    int blockers = 0;
    ATOMIC_COUNT_INCREMENTS(NUM_SAMPLES)
    for(int i = 0; i < NUM_SAMPLES; i++){
        ivec2 offset = ivec2(rotateM * samples[i] * searchRadius);
        float smDepth = textureOffset(sm, uv, offset).r;
        if(zReceiver >= smDepth){
            depthSum += smDepth;
            blockers++;
        }
    }
    if(blockers > 0) return depthSum / float(blockers);
    else return -1.0;
}
float PCF_Filter(sampler2D sm, vec2 uv, float zReceiver, float filterRadius, float bias) {
    float sum = 0.0;
    ATOMIC_COUNT_INCREMENTS(NUM_SAMPLES)
    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec2 os = rotateM * -samples[i].yx * filterRadius;
        float depth = texture(sm, uv + os).r;
        if (zReceiver <= depth + bias) sum += 1.0;
    }
    return sum / float(NUM_SAMPLES);
}
float PCSS_dir_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;

    float searchRadius = dirLights[index].light_size * dirLights[index].resolution;
    float avgDep = findBlocker(dirLights[index].shadowMap, ndc.xy, ndc.z, searchRadius);
    if(avgDep <= 0.0) return 1.0;
    float penumbraRatio = (ndc.z - avgDep) / avgDep;
    float filterSize = max(penumbraRatio * dirLights[index].light_size, 1.0 / dirLights[index].resolution);
    return PCF_Filter(dirLights[index].shadowMap, ndc.xy, ndc.z, filterSize, bias);
}
float PCSS_pt_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;

    float searchRadius = ptLights[index].light_size * ptLights[index].resolution;
    float avgDep = findBlocker(ptLights[index].shadowMap, ndc.xy, ndc.z, searchRadius);
    if(avgDep <= 0.0) return 1.0;
    float penumbraRatio = (ndc.z - avgDep) / avgDep;
    float filterSize = max(penumbraRatio * ptLights[index].light_size, 1.0 / ptLights[index].resolution);
    return PCF_Filter(ptLights[index].shadowMap, ndc.xy, ndc.z, filterSize, bias);
}
float PCF_dir_visible(int index, vec3 position, float bias) {
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;
    float filterRadius = SM_PCF_FILTER;
    return PCF_Filter(dirLights[index].shadowMap, ndc.xy, ndc.z, filterRadius, bias);
}
float PCF_pt_visible(int index, vec3 position, float bias) {
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;
    float filterRadius = SM_PCF_FILTER;
    return PCF_Filter(ptLights[index].shadowMap, ndc.xy, ndc.z, filterRadius, bias);
}
float ESM_dir_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;
    float mindep = texture(dirLights[index].shadowMap, ndc.xy).r;
    ATOMIC_COUNT_INCREMENT
    float sx = exp(-80.0 * ndc.z) * mindep;
    return clamp(sx, 0.0, 1.0);
}
float ESM_pt_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;
    float mindep = texture(ptLights[index].shadowMap, ndc.xy).r;
    ATOMIC_COUNT_INCREMENT
    float sx = exp(-80.0 * ndc.z) * mindep;
    return pow(clamp(sx, 0.0, 1.0), 2.0);
}
float HARD_dir_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;
    float mindep = texture(dirLights[index].shadowMap, ndc.xy).r;
    ATOMIC_COUNT_INCREMENT
    return step(ndc.z, mindep + bias);
}
float HARD_pt_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;
    float mindep = texture(ptLights[index].shadowMap, ndc.xy).r;
    ATOMIC_COUNT_INCREMENT
    return step(ndc.z, mindep + bias);
}

/******************************************Lighting***********************************************/
vec3 evalColor(ShadingData sd, LightSample ls){
    vec3 diffuse = vec3(0.0), specular = vec3(0.0);
    #ifdef EVAL_DIFFUSE
    vec3 diffuseBrdf = evalDiffuseBrdf(sd, ls);
    diffuse = ls.color * diffuseBrdf * ls.NdotL;
    #endif
    #ifdef EVAL_SPECULAR
    vec3 specularBrdf = evalSpecularBrdf(sd, ls);
    specular = ls.color * specularBrdf * ls.NdotL;
    #endif
    return diffuse + specular;
}

vec3 evalDirLight(int index, ShadingData sd){
    LightSample ls;
    ls.L = -normalize(dirLights[index].direction);
    ls.NdotL = dot(sd.N, ls.L);
    float visibility;
    if(ls.NdotL <= 0.0)
        visibility = 0.0;
    else if(dirLights[index].has_shadow){
        float bias = mix(MAX_BIAS, MIN_BIAS, ls.NdotL) / dirLights[index].resolution;
        #ifdef SHADOW_SOFT_EVSM
        visibility = EVSM_dir_visible(index, sd.posW, bias);
        #elif defined(SHADOW_SOFT_PCSS)
        visibility = PCSS_dir_visible(index, sd.posW, bias);
        #elif defined(SHADOW_SOFT_PCF)
        visibility = PCF_dir_visible(index, sd.posW, bias);
        #elif defined(SHADOW_SOFT_ESM)
        visibility = ESM_dir_visible(index, sd.posW, bias);
        #elif defined(SHADOW_HARD)
        visibility = HARD_dir_visible(index, sd.posW, bias);
        #else
        visibility = 1.0;
        #endif
    }else{
        visibility = 1.0;
    }
    vec3 H = normalize(sd.V + ls.L);
    ls.NdotH = dot(sd.N, H);
    ls.LdotH = dot(ls.L, H);
    ls.color = dirLights[index].intensity;
    vec3 ambient = dirLights[index].ambient * ls.color * sd.baseColor * sd.kD/* * sd.ao*/;
    if(visibility < EPS)
        return ambient;
    return visibility * evalColor(sd, ls) + ambient;
}

vec3 evalPtLight(int index, ShadingData sd){
    LightSample ls;
    ls.posW = ptLights[index].position;
    ls.L = normalize(ls.posW - sd.posW);
    ls.dist = length(ls.posW - sd.posW);
    float falloff = 1.0 / (ptLights[index].constant + ptLights[index].linear*ls.dist + ptLights[index].quadratic*ls.dist*ls.dist);
    float cosTheta = -dot(ls.L, normalize(ptLights[index].direction));
    if(ls.dist > ptLights[index].range || cosTheta < ptLights[index].cos_opening_angle)
        return vec3(0.0);
    else if(ptLights[index].penumbra_angle > 0.01){
        float deltaAngle = ptLights[index].opening_angle - acos(cosTheta);
        falloff *= clamp((deltaAngle - ptLights[index].penumbra_angle) / ptLights[index].penumbra_angle, 0.0, 1.0);
    }
    ls.NdotL = dot(sd.N, ls.L);
    float visibility;
    if(ls.NdotL <= 0.0)
        visibility = 0.0;
    else if(ptLights[index].has_shadow){
        float bias = max(MAX_BIAS * (1.0 - ls.NdotL), MIN_BIAS) / ptLights[index].resolution;
        #ifdef SHADOW_SOFT_EVSM
        visibility = EVSM_pt_visible(index, sd.posW, bias);
        #elif defined SHADOW_SOFT_PCSS
        visibility = PCSS_pt_visible(index, sd.posW, bias);
        #elif defined SHADOW_SOFT_PCF
        visibility = PCF_pt_visible(index, sd.posW, bias);
        #elif defined(SHADOW_SOFT_ESM)
        visibility = ESM_pt_visible(index, sd.posW, bias);
        #elif defined(SHADOW_HARD)
        visibility = HARD_pt_visible(index, sd.posW, bias);
        #else
        visibility = 1.0;
        #endif
    }else{
        visibility = 1.0;
    }
    vec3 H = normalize(sd.V + ls.L);
    ls.NdotH = dot(sd.N, H);
    ls.LdotH = dot(ls.L, H);
    ls.color = ptLights[index].intensity * falloff;
    vec3 ambient = ptLights[index].ambient * ls.color * sd.baseColor * sd.kD/* * sd.ao*/;
    if(visibility < EPS)
        return ambient;
    return visibility * evalColor(sd, ls) + ambient;
}

vec3 fixDirection(vec3 L, vec3 R, float range2){
    float RdotL = dot(L, R), L2 = dot(L, L);
    float a = sqrt(RdotL * RdotL + range2 - L2) - RdotL;
    return normalize(L + a * R);
}

vec3 evalIBL(int index, ShadingData sd, float weight){
    if(center_distance[index] >= 1.0)
        return vec3(0.0);
    vec3 L = sd.posW - ibls[index].position;
    float r2 = ibls[index].range * ibls[index].range;
    vec3 intensity = ibls[index].intensity * (1.0 - center_distance[index]) * weight;
    vec3 fixN = fixDirection(L, sd.N, r2);
    vec3 irradiance = textureLod(ibls[index].prefilterMap, fixN, ibls[index].miplevel).rgb;
    #ifdef IBL_GAMMACORRECTION
    irradiance = pow(irradiance, vec3(1.0/2.2));
    #endif
    vec3 diffuse = irradiance * sd.baseColor * sd.kD;

    vec3 fixR = fixDirection(L, sd.R, r2);
    vec3 prefilteredColor = textureLod(ibls[index].prefilterMap, fixR, sd.linearRoughness * (ibls[index].miplevel - 1.0)).rgb; 
    #ifdef IBL_GAMMACORRECTION
    prefilteredColor = pow(prefilteredColor, vec3(1.0/2.2));
    #endif
    vec2 envBRDF = texture(brdfLUT, vec2(min(sd.NdotV, 0.99), sd.linearRoughness)).rg;
    vec3 specular = prefilteredColor * (sd.kS * envBRDF.x + envBRDF.y);
    ATOMIC_COUNT_INCREMENTS(3)
    return (diffuse + specular) * intensity/* * sd.ao*/;
}

vec3 evalShading(vec3 baseColor, vec3 specColor, vec3 emissColor, vec3 normal, vec3 position, float linearDepth, float ao){
    ShadingData sd;
    sd.posW = position;
    sd.V = normalize(camera_pos - sd.posW);
    sd.N = normal;
    sd.R = reflect(-sd.V, sd.N);
    sd.NdotV = max(dot(sd.V, sd.N), 0.0);
    sd.lineardep = linearDepth;
    sd.ao = 1.0 - ao;
    sd.baseColor = baseColor;

    #if SHADING_MODEL == 1
    // Shading Model Metal Rough
    float IoR = 1.5;
    float f = (IoR - 1.0) / (IoR + 1.0);
    vec3 F0 = vec3(f * f);
    sd.metallic = specColor.b;
    sd.linearRoughness = specColor.g;
    sd.diffuse = mix(baseColor.rgb, vec3(0), specColor.b);
    sd.specular = mix(F0, baseColor.rgb, specColor.b);
    sd.ggxAlpha = max(0.0064, sd.linearRoughness * sd.linearRoughness);
    #elif SHADING_MODEL == 2
    sd.diffuse = baseColor.rgb;
    sd.specular = specColor.rgb;
    sd.linearRoughness = 0.0;
    sd.ggxAlpha = 0.0;
    #endif
    vec3 FS = fresnelSchlick(sd.specular, max(vec3(1.0 - sd.linearRoughness), sd.specular), sd.NdotV);
    float G = max(sd.NdotV, 0.01) / (max(sd.NdotV, 0.01) * (1.0 - sd.ggxAlpha) + sd.ggxAlpha);
    sd.kS = FS * G;
    sd.kD = 1.0 - FS;

    vec3 color = vec3(0.0, 0.0, 0.0);    
    int count = min(dirLtCount, MAX_DIRECTIONAL_LIGHT);
    for (int i = 0; i < count; i++){
        color += evalDirLight(i, sd);
    }
    count = min(ptLtCount, MAX_POINT_LIGHT);
    for (int i = 0; i < count; i++){
        color += evalPtLight(i, sd); 
    }
    #ifdef EVAL_IBL
    count = min(iblCount, MAX_IBL_LIGHT);
    if(count > 0){
        int min1 = 0, min2 = 0;
        for (int i = 0; i < count; i++){
            vec3 L = sd.posW - ibls[i].position;
            float r2 = ibls[i].range * ibls[i].range;
            center_distance[i] = dot(L, L) / r2;
            if(center_distance[i] < center_distance[min1])
                min1 = i;
        }
        if(count < 2)
            color += evalIBL(min1, sd, 1.0); 
        else{
            min2 = (min1 + 1) % count;
            for(int i = 0; i < count;i++)
                if(center_distance[i] < center_distance[min2] && center_distance[i] > center_distance[min1])
                    min2 = i;
            float weight1 = center_distance[min2] / (center_distance[min1] + center_distance[min2]);
            float weight2 = center_distance[min1] / (center_distance[min1] + center_distance[min2]);
            color += evalIBL(min1, sd, weight1); 
            color += evalIBL(min2, sd, weight2); 
        }
    }
    #endif
    color += emissColor;
    //color += emissColor;
    return color * sd.ao;
}
//sd.metallic = specColor.b;
//sd.linearRoughness = 1 - specColor.a;
//sd.ggxAlpha = max(0.0064, specColor.g * specColor.g);

    /*vec2 e = textureLod(dirLights[index].shadowMap, uv, 2).rg;
    if(ndc.z <= e.r + bias) return 1.0;
    float e2 = e.g - e.r * e.r;
    float pmax = e2 / (e2 + (ndc.z - e.r) * (ndc.z - e.r));
    return pmax;// VSM */