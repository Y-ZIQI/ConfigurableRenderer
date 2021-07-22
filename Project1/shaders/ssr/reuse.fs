//++`shaders/shading/defines.glsl`

out vec3 FragColor;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform sampler2D colorTex;
uniform sampler2D albedoTex;
uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;

uniform sampler2D hitPt12;
uniform sampler2D hitPt34;
uniform sampler2D hitPt56;
uniform sampler2D hitPt78;

#define MAX_SAMPLES 8

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
    /*vec3 specular = texture(specularTex, TexCoords).rgb;
    float roughness = specular.g;
    int kernel = int(roughness * 2.9);
    vec3 reflect_color = vec3(0.0);
    for(int x = -kernel; x <= kernel; x++){
        for(int y = -kernel; y <= kernel; y++){
            reflect_color += textureOffset(reflectionTex, TexCoords, ivec2(x, y)).rgb;
        }
    }
    reflect_color *= 1.0 / float((2 * kernel + 1) * (2 * kernel + 1));
    FragColor = texture(colorTex, TexCoords).rgb + reflect_color;*/
    //FragColor = vec3(texture(hitPt12, TexCoords).rg, 0.0);
    //FragColor = vec3(texture(colorTex, TexCoords).rgb);
    

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

    /*float r_weight = 1.0 / max(roughness, 0.01), weight_sum = 0.1, pdf;
    int num_samples = int(min(roughness, 0.99) * MAX_SAMPLES) + 1;
    vec2 hitpos;
    for(int i = 0; i < num_samples; i++){
        switch(i){
        case 0: hitpos = texture(hitPt12, TexCoords).xy; break;
        case 1: hitpos = texture(hitPt12, TexCoords).zw; break;
        case 2: hitpos = texture(hitPt34, TexCoords).xy; break;
        case 3: hitpos = texture(hitPt34, TexCoords).zw; break;
        case 4: hitpos = texture(hitPt56, TexCoords).xy; break;
        case 5: hitpos = texture(hitPt56, TexCoords).zw; break;
        case 6: hitpos = texture(hitPt78, TexCoords).xy; break;
        case 7: hitpos = texture(hitPt78, TexCoords).zw; break;
        }
        if(hitpos.x > -0.5){
            texPos = texture(positionTex, hitpos).xyz;
            H = normalize(normalize(texPos - position) - view);
            pdf = DistributionGGX(max(dot(H, N_mixed), 0.0), a2);
            reflect_color += r_weight * pdf * texture(colorTex, hitpos).rgb;
            weight_sum += r_weight * pdf;
        }else{
            weight_sum += r_weight;
        }
    }*/

    float weight_sum = 0.001, pdf;
    vec2 hitpos;
    for(int x = -1;x <= 1;x++){
        for(int y = -1;y <= 1;y++){
            float nroughness = textureOffset(specularTex, TexCoords, ivec2(x, y)).g;
            float r_weight = 1.0 / max(nroughness, 0.01);
            int num_samples = int(min(nroughness, 0.99) * MAX_SAMPLES) + 1;
            for(int i = 0; i < num_samples; i++){
                switch(i){
                case 0: hitpos = textureOffset(hitPt12, TexCoords, ivec2(x, y)).xy; break;
                case 1: hitpos = textureOffset(hitPt12, TexCoords, ivec2(x, y)).zw; break;
                case 2: hitpos = textureOffset(hitPt34, TexCoords, ivec2(x, y)).xy; break;
                case 3: hitpos = textureOffset(hitPt34, TexCoords, ivec2(x, y)).zw; break;
                case 4: hitpos = textureOffset(hitPt56, TexCoords, ivec2(x, y)).xy; break;
                case 5: hitpos = textureOffset(hitPt56, TexCoords, ivec2(x, y)).zw; break;
                case 6: hitpos = textureOffset(hitPt78, TexCoords, ivec2(x, y)).xy; break;
                case 7: hitpos = textureOffset(hitPt78, TexCoords, ivec2(x, y)).zw; break;
                }
                if(hitpos.x > -0.5){
                    texPos = texture(positionTex, hitpos).xyz;
                    H = normalize(normalize(texPos - position) - view);
                    pdf = DistributionGGX(max(dot(H, N_mixed), 0.0), a2);
                    reflect_color += r_weight * pdf * texture(colorTex, hitpos).rgb;
                    weight_sum += r_weight * pdf;
                }else{
                    weight_sum += r_weight;
                }
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
    FragColor = color + (1.0 - color) * reflect_color;
    //FragColor = vec3(texture(colorTex, TexCoords).rgb);
}