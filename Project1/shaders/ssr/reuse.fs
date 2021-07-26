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

uniform usampler2D hitPt12;
uniform usampler2D hitPt34;
uniform usampler2D hitPt56;
uniform usampler2D hitPt78;
uniform sampler2D weight1234;
uniform sampler2D weight5678;

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

void main(){
    vec3 specular = texture(specularTex, TexCoords).rgb;
    vec3 position = texture(positionTex, TexCoords).xyz, texPos, H;
    vec3 normal = texture(normalTex, TexCoords).rgb;
    float roughness = specular.g, ggxAlpha = max(0.0064, roughness * roughness), a2 = ggxAlpha * ggxAlpha;
    vec3 view = normalize(position - camera_pos);
    vec3 planeN = cross(normal, -view); // normal of incidence plane
    vec3 planeV = normalize(cross(planeN, normal)); // Vector of incidence plane
    vec3 N_revised = normalize(normalize(-view + planeV) + normalize(-view - planeV));
    vec3 N_mixed = normalize(mix(normal, N_revised, roughness * roughness));
    
    vec3 reflect_color = vec3(0.0);

    float weight_sum = 0.001, hitW, pdf;
    vec2 hitpos;
    uvec2 hitXY;
    for(int i = 0; i < RESOLVE; i++){
        float nroughness = textureOffset(specularTex, TexCoords, offsets[i]).g;
        float r_weight = 1.0 / max(nroughness, 0.01);
        int num_samples = int(min(nroughness, 0.99) * MAX_SAMPLES) + 1;
        for(int j = 0; j < num_samples; j++){
            switch(j){
            case 0: 
                hitXY = textureOffset(hitPt12, TexCoords, offsets[i]).xy;
                hitW = textureOffset(weight1234, TexCoords, offsets[i]).r; break;
            case 1: 
                hitXY = textureOffset(hitPt12, TexCoords, offsets[i]).zw; 
                hitW = textureOffset(weight1234, TexCoords, offsets[i]).g; break;
            case 2: 
                hitXY = textureOffset(hitPt34, TexCoords, offsets[i]).xy; 
                hitW = textureOffset(weight1234, TexCoords, offsets[i]).b; break;
            case 3: 
                hitXY = textureOffset(hitPt34, TexCoords, offsets[i]).zw; 
                hitW = textureOffset(weight1234, TexCoords, offsets[i]).a; break;
            case 4: 
                hitXY = textureOffset(hitPt56, TexCoords, offsets[i]).xy; 
                hitW = textureOffset(weight5678, TexCoords, offsets[i]).r; break;
            case 5: 
                hitXY = textureOffset(hitPt56, TexCoords, offsets[i]).zw; 
                hitW = textureOffset(weight5678, TexCoords, offsets[i]).g; break;
            case 6: 
                hitXY = textureOffset(hitPt78, TexCoords, offsets[i]).xy; 
                hitW = textureOffset(weight5678, TexCoords, offsets[i]).b; break;
            case 7: 
                hitXY = textureOffset(hitPt78, TexCoords, offsets[i]).zw; 
                hitW = textureOffset(weight5678, TexCoords, offsets[i]).a; break;
            }
            hitpos = vec2(hitXY) / vec2(width, height);
            float hit_weight = max(hitW, 0.5) * r_weight;
            if(hitW > 0.1){
                texPos = texture(positionTex, hitpos).xyz;
                H = normalize(normalize(texPos - position) - view);
                pdf = min(DistributionGGX(max(dot(H, N_mixed), 0.0), a2), 1.0);
                vec3 intensity = min(texture(colorTex, hitpos).rgb, 1.0);
                reflect_color += hitW * r_weight * pdf * intensity;
                weight_sum += hit_weight * pdf;
            }else{
                weight_sum += hit_weight;
            }
        }
    }
    
    vec3 albedo = texture(albedoTex, TexCoords).xyz;
    vec3 diffTerm = mix(albedo.rgb, vec3(0), specular.b);
    vec3 specTerm = mix(vec3(0.04), albedo, specular.b);
    float NdotV = max(dot(normal, -view), 0.0), G = NdotV / (NdotV * (1.0 - ggxAlpha) + ggxAlpha);
    vec3 reflectTerm = diffTerm * M_1_PI + fresnelSchlick(specTerm, vec3(1.0), NdotV) * G;
    
    reflect_color *= reflectTerm / weight_sum;
    vec3 color = vec3(texture(colorTex, TexCoords).rgb);
    FragColor = reflect_color;
    //FragColor = vec3(texture(colorTex, TexCoords).rgb);
}