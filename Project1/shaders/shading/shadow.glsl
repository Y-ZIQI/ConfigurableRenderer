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

float Linearize(float depth, float inv_n, float inv_f)
{
    float z = depth * 2.0 - 1.0;
    return 2.0 / (inv_n + inv_f - z * (inv_n - inv_f));    
}

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

void setRandomRotate(vec2 uv){
    float angle = random(uv) * M_2PI;
    rotateM = mat2(
        cos(angle), sin(angle),
        -sin(angle), cos(angle)
    );
}
float findBlocker(sampler2D sm, vec2 uv, float zReceiver, float searchRadius, int num_samples){
    float depthSum = 0.0;
    int blockers = 0;
    ATOMIC_COUNT_INCREMENTS(num_samples)
    int _step = NUM_SAMPLES/num_samples;
    for(int i = 0; i < NUM_SAMPLES; i += _step){
        //ivec2 offset = ivec2(rotateM * samples[i] * searchRadius);
        float smDepth = texture(sm, uv + rotateM * samples[i] * searchRadius).r;
        if(zReceiver >= smDepth){
            depthSum += smDepth;
            blockers++;
        }
    }
    if(blockers > 0) return depthSum / float(blockers);
    else return -1.0;
}
float PCF_Filter(sampler2D sm, vec2 uv, float zReceiver, float filterRadius, float bias, int num_samples) {
    float sum = 0.0;
    ATOMIC_COUNT_INCREMENTS(num_samples)
    int _step = NUM_SAMPLES/num_samples;
    for (int i = 0; i < NUM_SAMPLES; i += _step) {
        vec2 os = rotateM * -samples[i].yx * filterRadius;
        float depth = texture(sm, uv + os).r;
        if (zReceiver <= depth + bias) sum += 1.0;
    }
    return sum / float(num_samples);
}
float PCSS_dir_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;

    float searchRadius = dirLights[index].light_size;
    float avgDep = findBlocker(dirLights[index].shadowMap, ndc.xy, ndc.z, searchRadius, NUM_SAMPLES);
    if(avgDep <= 0.0) return 1.0;
    float penumbraRatio = (ndc.z - avgDep) / avgDep;
    float filterSize = max(penumbraRatio * dirLights[index].light_size, 1.0 / dirLights[index].resolution);
    return PCF_Filter(dirLights[index].shadowMap, ndc.xy, ndc.z, filterSize, bias, NUM_SAMPLES);
}
float PCSS_pt_visible(int index, vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;

    float searchRadius = ptLights[index].light_size;
    float avgDep = findBlocker(ptLights[index].shadowMap, ndc.xy, ndc.z, searchRadius, 10);
    if(avgDep <= 0.0) return 1.0;
    float lineard = Linearize(avgDep, ptLights[index].inv_n, ptLights[index].inv_f);
    float linearz = Linearize(ndc.z, ptLights[index].inv_n, ptLights[index].inv_f);
    float penumbraRatio = (linearz - lineard) / lineard;
    float filterSize = max(penumbraRatio * ptLights[index].light_size, 1.0 / ptLights[index].resolution);
    return PCF_Filter(ptLights[index].shadowMap, ndc.xy, ndc.z, filterSize, bias, 10);
}
float PCF_dir_visible(int index, vec3 position, float bias) {
    vec3 ndc;
    if(!posFromLight(dirLights[index].viewProj, position, ndc)) return 1.0;
    float filterRadius = SM_PCF_FILTER;
    return PCF_Filter(dirLights[index].shadowMap, ndc.xy, ndc.z, filterRadius, bias, NUM_SAMPLES);
}
float PCF_pt_visible(int index, vec3 position, float bias) {
    vec3 ndc;
    if(!posFromLight(ptLights[index].viewProj, position, ndc)) return 1.0;
    float filterRadius = SM_PCF_FILTER;
    return PCF_Filter(ptLights[index].shadowMap, ndc.xy, ndc.z, filterRadius, bias, NUM_SAMPLES);
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