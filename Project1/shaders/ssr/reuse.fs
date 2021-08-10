//++`shaders/shading/defines.glsl`

#ifndef SSR_LEVEL
#define SSR_LEVEL 2
#endif

layout (location = 0) out vec3 FragColor;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform int width;
uniform int height;

uniform sampler2D colorTex;
uniform sampler2D albedoTex;
uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;

uniform usampler2D hitPt1234;
uniform usampler2D hitPt5678;

#if SSR_LEVEL == 1
#define MAX_SAMPLES 6
#define RESOLVE 5
#else  // SSR_LEVEL == 2
#define MAX_SAMPLES 8
#define RESOLVE 9
#endif

const ivec2 offsets[] = {
    ivec2(0, 0), ivec2(-1, 0), ivec2(0, 1), 
    ivec2(0, -1), ivec2(0, 1), ivec2(-1, -1), 
    ivec2(-1, 1), ivec2(1, -1), ivec2(1, 1)
};

float DistributionGGX(float NdotH, float a2)
{
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (d * d);
}

vec3 fresnelSchlick(vec3 f0, vec3 f90, float u)
{
    return f0 + (f90 - f0) * pow(1 - u, 5);
}

uvec2 unpack1to2(uint a){
    return uvec2((a >> 16) & 0x0000ffffu, a & 0x0000ffffu);
}

void main(){
    vec3 specular = texture(specularTex, TexCoords).rgb;
    vec3 position = texture(positionTex, TexCoords).xyz, texPos, H;
    vec3 normal = texture(normalTex, TexCoords).rgb;
    float roughness = specular.g, ggxAlpha = max(0.0064, roughness * roughness), a2 = ggxAlpha * ggxAlpha;
    vec3 view = normalize(position - camera_pos);
    
    vec3 reflect_color = vec3(0.0);
    float weight_sum = 1.0, pdf;
    uvec4 _hitXY;
    uint packXY;
    uvec2 hitXY;
    vec2 hitpos;
    for(int i = 0; i < RESOLVE; i++){
        float nroughness = textureOffset(specularTex, TexCoords, offsets[i]).g;
        float r_weight = 1.0 / max(nroughness, 0.1);
        int num_samples = int(ceil(max(sqrt(roughness) * MAX_SAMPLES, 1.0)));
        //int num_samples = int(min(sqrt(nroughness), 0.99) * MAX_SAMPLES) + 1;
        ATOMIC_COUNT_INCREMENTS((num_samples / 4) + 1)
        for(int j = 0; j < num_samples; j++){
            switch(j){
            case 0: 
                _hitXY = textureOffset(hitPt1234, TexCoords, offsets[i]);
                packXY = _hitXY.r; break;
            case 1: packXY = _hitXY.g; break;
            case 2: packXY = _hitXY.b; break;
            case 3: packXY = _hitXY.a; break;
            case 4: 
                _hitXY = textureOffset(hitPt5678, TexCoords, offsets[i]); 
                packXY = _hitXY.r; break;
            case 5: packXY = _hitXY.g; break;
            case 6: packXY = _hitXY.b; break;
            case 7: packXY = _hitXY.a; break;
            }
            if(packXY > 0){
                ATOMIC_COUNT_INCREMENTS(2)
                hitXY = unpack1to2(packXY);
                hitpos = vec2(hitXY - 1) / vec2(width, height);
                texPos = texture(positionTex, hitpos).xyz;
                H = normalize(normalize(texPos - position) - view);
                pdf = min(DistributionGGX(max(dot(H, normal), 0.0), a2), 1.0);
                vec3 intensity = min(texture(colorTex, hitpos).rgb, 10.0);
                reflect_color += r_weight * pdf * intensity;
                weight_sum += r_weight * pdf;
            }else{
                weight_sum += r_weight;
            }
        }
    }
    
    vec3 albedo = texture(albedoTex, TexCoords).xyz;
    vec3 diffTerm = mix(albedo.rgb, vec3(0), specular.b);
    vec3 specTerm = mix(vec3(0.04), albedo, specular.b);
    float NdotV = max(dot(normal, -view), 0.0), G = NdotV / (NdotV * (1.0 - ggxAlpha) + ggxAlpha);
    vec3 reflectTerm = diffTerm + fresnelSchlick(specTerm, vec3(1.0), NdotV) * G;
    
    reflect_color *= reflectTerm * M_1_PI / weight_sum;
    vec3 color = vec3(texture(colorTex, TexCoords).rgb);
    FragColor = reflect_color;
    //FragColor = vec3(texture(colorTex, TexCoords).rgb);
    ATOMIC_COUNT_INCREMENTS(5)
    ATOMIC_COUNT_CALCULATE
}