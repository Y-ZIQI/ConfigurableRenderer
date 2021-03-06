//++`shaders/shading/defines.glsl`
//++`shaders/shading/importanceSample.glsl`
//++`shaders/shading/noise.glsl`

#ifndef SSR_LEVEL
#define SSR_LEVEL 2
#endif

layout (location = 0) out uvec4 hitPt1234;
layout (location = 1) out uvec4 hitPt5678;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform int width;
uniform int height;
uniform float threshold;

uniform sampler2D rayamountTex;
uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D lineardepthTex;

#if SSR_LEVEL == 1
#define MAX_SAMPLES 6
const float init_steps[] = {
    5, 10, 30, 50, 70, 90, 110, 130, 160, 200
};
#else // SSR_LEVEL == 2
#define MAX_SAMPLES 8
const float init_steps[] = {
    5, 10, 20, 30, 40, 50, 60, 70, 80, 100
};
#endif

//#define ELONGATION 2.0

uniform vec2 samples[MAX_SAMPLES];
mat3 rotateM;

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
bool rayTrace_screenspace(vec3 texPos, vec3 texDir, float init_step, out vec2 hitPos){
    vec3 one_step = texDir * init_step;
    float linearz = Linearize(texPos.z), lineard, distz, d1, d2;
    uint max_step = uint(min(min(
        max(-texPos.x / one_step.x, (1.0 - texPos.x) / one_step.x), 
        max(-texPos.y / one_step.y, (1.0 - texPos.y) / one_step.y)
    ),  max(-texPos.z / one_step.z, (1.0 - texPos.z) / one_step.z)
    )) + 1;
    
    texPos += one_step * random(texDir.yx);
    distz = Linearize(texPos.z) - linearz;
    d1 = distz;
    linearz += distz;
    ATOMIC_COUNT_INCREMENT
    lineard = texture(lineardepthTex, texPos.xy).r;
    for(uint i = 0;i < max_step;i++){
        if(linearz > lineard + 0.001){
            d2 = linearz - lineard;
            if(d2 < threshold + max(distz, 0.0)){
                hitPos = texPos.xy - one_step.xy * d2 / (abs(d1) + d2);
                return true;
            }
        }
        d1 = lineard - linearz;
        texPos += one_step;
        distz = Linearize(texPos.z) - linearz;
        linearz += distz;
        ATOMIC_COUNT_INCREMENT
        lineard = texture(lineardepthTex, texPos.xy).r;
    }
    return false;
}

uint pack2to1(uvec2 uv){
    return ((uv.x << 16) & (0xffff0000u)) | (uv.y & 0x0000ffffu);
}

void main(){
    vec3 normal = texture(normalTex, TexCoords).rgb;
    hitPt1234 = uvec4(0);
    hitPt5678 = uvec4(0);
    if(dot(normal, normal) == 0.0){
        return;
    }
    vec3 position = texture(positionTex, TexCoords).xyz;
    vec3 specular = texture(specularTex, TexCoords).rgb;
    vec3 view = normalize(position - camera_pos);
    float roughness = specular.g;
    mat3 TBN;
    TBN[2] = normal;
    vec3 up = abs(normal.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    TBN[0] = normalize(cross(up, normal));
    TBN[1] = cross(normal, TBN[0]);
    
    float angle = random(TexCoords) * M_2PI, move = random(TexCoords.yx) * 0.12;
    rotateM = mat3(
        cos(angle), sin(angle), 0.0,
        -sin(angle), cos(angle), 0.0,
        0.0, 0.0, 1.0
    );
    TBN = TBN * rotateM;

    int level = int(roughness * 9.9);
    float init_step = init_steps[level] / 1000.0, pdf;
    float rayAmount1 = max(textureOffset(rayamountTex, TexCoords, ivec2(-1, 0)).r, textureOffset(rayamountTex, TexCoords, ivec2(1, 0)).r);
    float rayAmount2 = max(textureOffset(rayamountTex, TexCoords, ivec2(0, -1)).r, textureOffset(rayamountTex, TexCoords, ivec2(0, 1)).r);
    float rayAmount = max(max(rayAmount1, rayAmount2), texture(rayamountTex, TexCoords).r);
    rayAmount = max(rayAmount - 0.15, 0.0);
    int num_samples = int(ceil(max(sqrt(roughness) * MAX_SAMPLES, 1.0) * rayAmount));
    //int num_samples = int(min(roughness, 0.99) * MAX_SAMPLES) + 1;
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
        bool intersect = rayTrace_screenspace(texPos, texDir, init_step, hitpos);
        uint packXY = intersect? pack2to1(uvec2(vec2(width, height) * hitpos) + 1) : 0;
        switch(i){
        case 0: hitPt1234.r = packXY; break;
        case 1: hitPt1234.g = packXY; break;
        case 2: hitPt1234.b = packXY; break;
        case 3: hitPt1234.a = packXY; break;
        case 4: hitPt5678.r = packXY; break;
        case 5: hitPt5678.g = packXY; break;
        case 6: hitPt5678.b = packXY; break;
        case 7: hitPt5678.a = packXY; break;
        }
    }
    ATOMIC_COUNT_INCREMENTS(8)
    ATOMIC_COUNT_CALCULATE
}