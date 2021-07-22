//++`shaders/shading/defines.glsl`
//++`shaders/shading/importanceSample.glsl`

layout (location = 0) out vec4 hitPt12;
layout (location = 1) out vec4 hitPt34;
layout (location = 2) out vec4 hitPt56;
layout (location = 3) out vec4 hitPt78;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;
uniform sampler2D lineardepthTex;

#define MAX_SAMPLES 8

uniform vec2 samples[MAX_SAMPLES];
mat3 rotateM;

#define MAX_DISTANCE 0.3
const float thresholds[] = {
    0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.8, 1.0
};

const float init_steps[] = {
    1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0
};

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
bool rayTrace_screenspace(vec3 texPos, vec3 texDir, float roughness, float init_step, float threshold, out vec2 hitPos){
    vec3 one_step = texDir * init_step;
    float linearz = Linearize(texPos.z), lineard, distz, d1, d2;
    uint max_step = uint(min(min(
        max(-texPos.x / one_step.x, (1.0 - texPos.x) / one_step.x), 
        max(-texPos.y / one_step.y, (1.0 - texPos.y) / one_step.y)
    ),  max(-texPos.z / one_step.z, (1.0 - texPos.z) / one_step.z)
    ));
    
    vec2 candidate = vec2(-1.0);
    float mindist = 999.0;
    texPos += one_step;
    distz = Linearize(texPos.z) - linearz;
    d1 = distz;
    linearz += distz;
    lineard = texture(lineardepthTex, texPos.xy).r;
    for(uint i = 0;i < max_step;i++){
        if(linearz > lineard + 0.001){
            d2 = linearz - lineard;
            if(d2 < threshold + max(distz, 0.0)){
                hitPos = texPos.xy - one_step.xy * d2 / (abs(d1) + d2);
                return true;
            }else if(d2 < min(MAX_DISTANCE, mindist)){
                candidate = texPos.xy - one_step.xy * d2 / (abs(d1) + d2);
                mindist = d2;        
            }
        }
        d1 = lineard - linearz;
        texPos += one_step;
        distz = Linearize(texPos.z) - linearz;
        linearz += distz;
        lineard = texture(lineardepthTex, texPos.xy).r;
    }
    if(mindist < MAX_DISTANCE) hitPos = candidate;
    else hitPos = vec2(-1.0);
    return false;
}

void main(){
    ATOMIC_COUNT_INCREMENT
    vec3 normal = texture(normalTex, TexCoords).rgb;
    hitPt12 = vec4(-1.0, -1.0, -1.0, -1.0);
    hitPt34 = vec4(-1.0, -1.0, -1.0, -1.0);
    hitPt56 = vec4(-1.0, -1.0, -1.0, -1.0);
    hitPt78 = vec4(-1.0, -1.0, -1.0, -1.0);
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
    float threshold = thresholds[level], pdf;
    float init_step = init_steps[level] / 100.0;
    int num_samples = int(min(roughness, 0.99) * MAX_SAMPLES) + 1;
    vec2 hitpos;
    vec3 texPos = getScreenUV(position);
    for(int i = 0; i < num_samples; i++){
        vec2 nXi = samples[i];
        nXi.x += move;
        vec3 H = visibleImportanceSampling(nXi, TBN, roughness, pdf);
        vec3 refv = reflect(view, H);
        vec3 texDir = getScreenUV(position + refv * 0.1) - texPos;
        texDir *= 1.0 / length(vec2(texDir.xy));
        bool inter = rayTrace_screenspace(texPos, texDir, roughness, init_step, threshold, hitpos);
        switch(i){
        case 0: hitPt12.xy = hitpos; break;
        case 1: hitPt12.zw = hitpos; break;
        case 2: hitPt34.xy = hitpos; break;
        case 3: hitPt34.zw = hitpos; break;
        case 4: hitPt56.xy = hitpos; break;
        case 5: hitPt56.zw = hitpos; break;
        case 6: hitPt78.xy = hitpos; break;
        case 7: hitPt78.zw = hitpos; break;
        }
    }
}

                /*vec3 backPos = texPos - one_step * d2 / (abs(d1) + d2);
                linearz = Linearize(backPos.z);
                lineard = texture(lineardepthTex, hitPos).r;
                if(abs(linearz - lineard) > d2) hitPos = texPos.xy;
                else hitPos = backPos.xy;*/