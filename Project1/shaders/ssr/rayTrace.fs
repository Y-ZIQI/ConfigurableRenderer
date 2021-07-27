//++`shaders/shading/defines.glsl`
//++`shaders/shading/importanceSample.glsl`

#ifndef SSR_LEVEL
#define SSR_LEVEL 2
#endif

layout (location = 0) out uvec4 hitPt12;
layout (location = 1) out uvec4 hitPt34;
layout (location = 2) out uvec4 hitPt56;
layout (location = 3) out uvec4 hitPt78;
layout (location = 4) out vec4 weight1234;
layout (location = 5) out vec4 weight5678;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform int width;
uniform int height;
uniform float threshold;

uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;
uniform sampler2D lineardepthTex;

#if SSR_LEVEL == 1
#define MAX_SAMPLES 6
#define MAX_DISTANCE 0.5
const float init_steps[] = {
    2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0
};
#else // SSR_LEVEL == 2
#define MAX_SAMPLES 8
#define MAX_DISTANCE 0.5
const float init_steps[] = {
    1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0
};
#endif

/*const float thresholds[] = {
    0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.8, 1.0
};*/
//#define ELONGATION 2.0

uniform vec2 samples[MAX_SAMPLES];
mat3 rotateM;

float Rand1(inout float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float Linearize(float depth)
{
    float z = depth * 2.0 - 1.0;
    return 2.0 / (camera_params.z - z * camera_params.w);    
}

// From world pos to [screen.xy, depth]
vec3 getScreenUV(vec3 posW){
    vec4 buf = camera_vp * vec4(posW, 1.0);
    return buf.xyz * (0.5 / buf.w) + 0.5;
}

// Need Optimize
float rayTrace_screenspace(vec3 texPos, vec3 texDir, float init_step, out vec2 hitPos){
    vec3 one_step = texDir * init_step;
    float linearz = Linearize(texPos.z), lineard, distz, d1, d2;
    uint max_step = uint(min(min(
        max(-texPos.x / one_step.x, (1.0 - texPos.x) / one_step.x), 
        max(-texPos.y / one_step.y, (1.0 - texPos.y) / one_step.y)
    ),  max(-texPos.z / one_step.z, (1.0 - texPos.z) / one_step.z)
    ));
    
    texPos += one_step;
    distz = Linearize(texPos.z) - linearz;
    d1 = distz;
    linearz += distz;
    ATOMIC_COUNT_INCREMENT
    lineard = texture(lineardepthTex, texPos.xy).r;
    for(uint i = 0;i < max_step;i++){
        if(linearz > lineard + 0.001){
            d2 = linearz - lineard;
            if(d2 < threshold + abs(distz)){
                hitPos = texPos.xy - one_step.xy * d2 / (abs(d1) + d2);
                return 1.0;
            }
        }
        d1 = lineard - linearz;
        texPos += one_step;
        distz = Linearize(texPos.z) - linearz;
        linearz += distz;
        ATOMIC_COUNT_INCREMENT
        lineard = texture(lineardepthTex, texPos.xy).r;
    }
    texPos = clamp(texPos, 0.0, 1.0);
    linearz = Linearize(texPos.z);
    ATOMIC_COUNT_INCREMENT
    lineard = texture(lineardepthTex, texPos.xy).r;
    if(lineard >= Linearize(1.0) - 0.1) return 0.0;
    hitPos = texPos.xy;
    float dist_w = abs(distz) / (abs(lineard - linearz) + abs(distz));
    return dist_w;
}

void main(){
    vec3 normal = texture(normalTex, TexCoords).rgb;
    weight1234 = vec4(0.0);
    weight5678 = vec4(0.0);
    if(dot(normal, normal) == 0.0){
        return;
    }
    vec3 position = texture(positionTex, TexCoords).xyz;
    vec3 specular = texture(specularTex, TexCoords).rgb;
    vec3 view = normalize(position - camera_pos);
    float roughness = specular.g;
    vec3 planeN = cross(normal, -view); // normal of incidence plane
    vec3 planeV = normalize(cross(planeN, normal)); // Vector of incidence plane
    vec3 N_revised = normalize(normalize(-view + planeV) + normalize(-view - planeV));
    vec3 N_mixed = normalize(mix(normal, N_revised, roughness * roughness));
    mat3 TBN;
    TBN[2] = N_mixed;
    vec3 up = abs(N_mixed.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    TBN[0] = normalize(cross(up, N_mixed));
    TBN[1] = cross(N_mixed, TBN[0]);
    
    float seed = InitRand(TexCoords); 
    float angle = Rand1(seed) * M_2PI, move = Rand1(seed) * 0.12;
    rotateM = mat3(
        cos(angle), sin(angle), 0.0,
        -sin(angle), cos(angle), 0.0,
        0.0, 0.0, 1.0
    );
    TBN = TBN * rotateM;

    int level = int(roughness * 9.9);
    float init_step = init_steps[level] / 100.0, pdf;
    int num_samples = int(min(roughness, 0.99) * MAX_SAMPLES) + 1;
    vec2 hitpos;
    vec3 texPos = getScreenUV(position);
    for(int i = 0; i < num_samples; i++){
        vec2 nXi = samples[i];
        nXi.x += move;
        vec3 H = visibleImportanceSampling(nXi, TBN, roughness, pdf);
        #ifdef ELONGATION
        float vertical = dot(planeV, H);
        vec3 Hvertical = planeV * vertical;
        H = normalize(H + (ELONGATION - 1.0) * Hvertical);
        #endif
        vec3 refv = reflect(view, H);
        vec3 texDir = getScreenUV(position + refv * 0.1) - texPos;
        texDir *= 1.0 / length(vec2(texDir.xy));
        float inter = rayTrace_screenspace(texPos, texDir, init_step, hitpos);
        ivec2 hitXY = ivec2(vec2(width, height) * hitpos);
        switch(i){
        case 0: hitPt12.xy = hitXY; weight1234.r = inter; break;
        case 1: hitPt12.zw = hitXY; weight1234.g = inter; break;
        case 2: hitPt34.xy = hitXY; weight1234.b = inter; break;
        case 3: hitPt34.zw = hitXY; weight1234.a = inter; break;
        case 4: hitPt56.xy = hitXY; weight5678.r = inter; break;
        case 5: hitPt56.zw = hitXY; weight5678.g = inter; break;
        case 6: hitPt78.xy = hitXY; weight5678.b = inter; break;
        case 7: hitPt78.zw = hitXY; weight5678.a = inter; break;
        }
    }
    ATOMIC_COUNT_INCREMENTS(3)
    ATOMIC_COUNT_CALCULATE
}

                /*vec3 backPos = texPos - one_step * d2 / (abs(d1) + d2);
                linearz = Linearize(backPos.z);
                lineard = texture(lineardepthTex, hitPos).r;
                if(abs(linearz - lineard) > d2) hitPos = texPos.xy;
                else hitPos = backPos.xy;*/