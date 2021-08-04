//++`shaders/shading/defines.glsl`
//++`shaders/shading/importanceSample.glsl`
//++`shaders/shading/noise.glsl`

//out float rayAmount;
out vec3 outColor;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform float threshold;

uniform sampler2D albedoTex;
uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D lineardepthTex;
uniform sampler2D colorTex;

#define MAX_AMOUNT 8
#define MAX_SAMPLES 8
const float init_steps[] = {
    1, 1, 2, 3, 4, 5, 6, 7, 8, 10
};

uniform vec2 samples[MAX_SAMPLES];
mat3 rotateM;

float Linearize(float depth)
{
    float z = depth * 2.0 - 1.0;
    return 2.0 / (camera_params.z - z * camera_params.w);    
}

vec3 fresnelSchlick(vec3 f0, vec3 f90, float u)
{
    return f0 + (f90 - f0) * pow(1 - u, 5);
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
    if(dot(normal, normal) == 0.0){
        //rayAmount = 0.0;
        outColor = vec3(0.0);
        return;
    }
    vec3 position = texture(positionTex, TexCoords).xyz;
    vec3 specular = texture(specularTex, TexCoords).rgb;
    vec3 view = normalize(position - camera_pos);
    float roughness = specular.g, ggxAlpha = max(0.0064, roughness * roughness), a2 = ggxAlpha * ggxAlpha;
    vec3 planeN = cross(normal, -view); // normal of incidence plane
    vec3 planeV = normalize(cross(planeN, normal)); // Vector of incidence plane
    vec3 N_revised = normalize(normalize(-view + planeV) + normalize(-view - planeV));
    vec3 N_mixed = normalize(mix(normal, N_revised, roughness * roughness));
    mat3 TBN;
    TBN[2] = N_mixed;
    vec3 up = abs(N_mixed.z) < 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    TBN[0] = normalize(cross(up, N_mixed));
    TBN[1] = cross(N_mixed, TBN[0]);
    
    float angle = random(TexCoords) * M_2PI, move = random(TexCoords.yx) * 0.12;
    rotateM = mat3(
        cos(angle), sin(angle), 0.0,
        -sin(angle), cos(angle), 0.0,
        0.0, 0.0, 1.0
    );
    TBN = TBN * rotateM;

    int level = int(roughness * 9.9);
    float init_step = init_steps[level] / 100.0, pdf;
    int num_samples = int(ceil(max(sqrt(roughness) * MAX_SAMPLES, 1.0)));
    //int num_samples = int(min(roughness, 0.99) * MAX_SAMPLES) + 1;
    vec2 hitpos;
    vec3 texPos = getScreenUV(position);

    float weight_sum = 0.0;
    vec3 reflect_color = vec3(0.0);
    for(int i = 0; i < num_samples; i++){
        vec2 nXi = samples[i];
        nXi.x += move;
        vec3 H = visibleImportanceSampling(nXi, TBN, roughness, pdf);
        vec3 refv = reflect(view, H);
        vec3 texDir = getScreenUV(position + refv * 0.1) - texPos;
        texDir *= 1.0 / length(vec2(texDir.xy));
        float inter = rayTrace_screenspace(texPos, texDir, init_step, hitpos);
        reflect_color += texture(colorTex, hitpos).rgb * inter;
    }

    vec3 albedo = texture(albedoTex, TexCoords).xyz;
    vec3 diffTerm = mix(albedo.rgb, vec3(0), specular.b);
    vec3 specTerm = mix(vec3(0.04), albedo, specular.b);
    float NdotV = max(dot(normal, -view), 0.0), G = NdotV / (NdotV * (1.0 - ggxAlpha) + ggxAlpha);
    vec3 reflectTerm = diffTerm + fresnelSchlick(specTerm, vec3(1.0), NdotV) * G;
    
    reflect_color *= reflectTerm * M_1_PI / float(num_samples);
    vec3 color = texture(colorTex, TexCoords).rgb;
    float weight = min(length(reflect_color) / max(length(color), 0.01), 1.0);
    outColor = vec3(weight, 0.0, 0.0);
    ATOMIC_COUNT_INCREMENTS(5 + num_samples)
    ATOMIC_COUNT_CALCULATE
}
