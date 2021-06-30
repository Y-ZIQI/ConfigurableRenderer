//++`shaders/shading/defines.glsl`

#ifndef SAMPLE_NUM
#define SAMPLE_NUM 16
#endif
#ifndef SSAO_RANGE
#define SSAO_RANGE 0.4
#endif
#ifndef SSAO_THRESHOLD
#define SSAO_THRESHOLD 1.0
#endif
#ifndef SAMPLE_BIAS
#define SAMPLE_BIAS 0.05
#endif

out float AO;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform sampler2D positionTex;
uniform sampler2D normalTex;

uniform vec3 samples[SAMPLE_NUM];
//uniform sampler2D noiseTex;

float Rand1(inout float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec2 Rand2(inout float p) {
    return vec2(Rand1(p), Rand1(p));
}

vec3 Rand3(inout float p) {
    return vec3(Rand1(p), Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
    vec2 uv = Rand2(s);
    float z = uv.x;
    float phi = uv.y * M_2PI;
    float sinTheta = sqrt(1.0 - z*z);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    pdf = M_1_2PI;
    return dir;
}

vec3 SampleHemisphere(inout float s) {
    vec3 uv = Rand3(s);
    float z = sqrt(1.0 - uv.x);
    float phi = uv.y * M_2PI;
    float sinTheta = sqrt(uv.x);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    return dir * uv.z;
}

void LocalBasis(vec3 n, vec3 randVec, out vec3 b1, out vec3 b2) {
    b1 = normalize(randVec - n * dot(randVec, n));
    b2 = cross(n, b1);
}

float LinearizeDepth(float depth, vec2 inv_nf)
{
    float z = depth * 2.0 - 1.0; // 回到NDC
    return 2.0 / (inv_nf.x - z * inv_nf.y);    
}

float ambientOcclusion(vec3 posW, vec3 normal, float range, float threshold, mat4 transform, vec2 inv_nf, sampler2D positionTex){
    //float seed = InitRand(vec2(1.0));
    float seed = InitRand(posW.xy);
    float sum = 0.0, mindep, dist;
    vec3 randPos, samplePos, objPos, b1, b2, ndc;
    vec4 ssPos;

    LocalBasis(normal, Rand3(seed) * 2.0 - 1.0, b1, b2);
    mat3 TBN = mat3(b1, b2, normal);
    for(int i = 0;i < SAMPLE_NUM;i++){
        //randPos = SampleHemisphere(seed) * range;
        randPos = samples[i] * range;
        samplePos = TBN * randPos + posW;
        ssPos = transform * vec4(samplePos, 1.0);
        ndc = clamp((ssPos.xyz / ssPos.w + 1.0) / 2.0, 0.0, 1.0);
        ATOMIC_COUNT_INCREMENT
        mindep = texture(positionTex, ndc.xy).w;
        ndc.z = LinearizeDepth(ndc.z, inv_nf);
        sum += step(ndc.z, mindep + SAMPLE_BIAS) + step(mindep + threshold, ndc.z);
    }
    return 1.0 - sum / float(SAMPLE_NUM);
}

void main(){
    ATOMIC_COUNT_INCREMENT
    vec3 normal = texture(normalTex, TexCoords).xyz;
    if(dot(normal, normal) == 0.0){
        AO = 0.0;
    }else{
        ATOMIC_COUNT_INCREMENT
        vec3 posW = texture(positionTex, TexCoords).xyz;
        AO = ambientOcclusion(posW, normal, SSAO_RANGE, SSAO_THRESHOLD, camera_vp, camera_params.zw, positionTex);
    }
}