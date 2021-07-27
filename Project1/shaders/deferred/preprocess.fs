//++`shaders/shading/defines.glsl`

#ifndef SAMPLE_NUM
#define SAMPLE_NUM 16
#endif

layout (location = 0) out vec3 fNormal;
layout (location = 1) out float fDepth;
layout (location = 2) out float AO;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform float ssao_range;
uniform float ssao_bias;
uniform float ssao_threshold;

uniform sampler2D positionTex;
uniform sampler2D tangentTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;

uniform vec3 samples[SAMPLE_NUM];

float Rand1(inout float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec3 Rand3(inout float p) {
    return vec3(Rand1(p), Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return 2.0 / (camera_params.z - z * camera_params.w);    
}

void LocalBasis(vec3 n, vec3 randVec, out vec3 b1, out vec3 b2) {
    b1 = normalize(randVec - n * dot(randVec, n));
    b2 = cross(n, b1);
}

float ambientOcclusion(vec3 posW, vec3 normal){
    float seed = InitRand(TexCoords * 100.0);
    float sum = 0.0, mindep, dist;
    vec3 randPos, samplePos, objPos, b1, b2, ndc;
    vec4 ssPos;

    LocalBasis(normal, Rand3(seed) * 2.0 - 1.0, b1, b2);
    mat3 TBN = mat3(b1, b2, normal);
    for(int i = 0;i < SAMPLE_NUM;i++){
        randPos = samples[i] * ssao_range;
        samplePos = TBN * randPos + posW;
        ssPos = camera_vp * vec4(samplePos, 1.0);
        ndc = clamp((ssPos.xyz / ssPos.w + 1.0) / 2.0, 0.0, 1.0);
        ATOMIC_COUNT_INCREMENT
        mindep = LinearizeDepth(texture(depthTex, ndc.xy).x);
        ndc.z = LinearizeDepth(ndc.z);
        sum += step(ndc.z, mindep + ssao_bias) + step(mindep + ssao_threshold, ndc.z);
    }
    return 1.0 - sum / float(SAMPLE_NUM);
}

void main(){
    vec3 position = texture(positionTex, TexCoords).xyz;
    float depth = texture(depthTex, TexCoords).x;
    fDepth = LinearizeDepth(depth);
    vec4 tangent = texture(tangentTex, TexCoords);
    vec4 normal = texture(normalTex, TexCoords);
    if(dot(normal.xyz, normal.xyz) <= 1e-8){
        fNormal = vec3(0.0, 0.0, 0.0);
        AO = 0.0;
    }else{
        mat3 TBN = mat3(
            tangent.xyz,
            cross(normal.xyz, tangent.xyz),
            normal.xyz
        );
        vec3 N = vec3(tangent.w, normal.w, 0.0);
        N.z = sqrt(1.0 - clamp(dot(N, N), 0.0, 1.0));
        N.xy = N.xy * 2.0 - 1.0;
        N = normalize(TBN * N);
        fNormal = N;
        #ifdef SSAO
        AO = ambientOcclusion(position, N);
        #else
        AO = 0.0;
        #endif
    }
    ATOMIC_COUNT_INCREMENTS(4)
    ATOMIC_COUNT_CALCULATE
}