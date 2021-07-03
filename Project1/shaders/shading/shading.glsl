//++`shaders/shading/BRDF.glsl`

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

struct DirectionalLight{
    vec3 intensity;
    vec3 direction;
    float ambient;

    bool has_shadow;
    mat4 viewProj;
    sampler2D shadowMap;
    float resolution;
};

struct PointLight{
    vec3 intensity;
    vec3 position;
    vec3 direction;
    float ambient;

    float range;
    float opening_angle, cos_opening_angle;
    float penumbra_angle;

    float constant;
    float linear;
    float quadratic;

    bool has_shadow;
    mat4 viewProj;
    sampler2D shadowMap;
    float resolution;
};

#ifndef MAX_DIRECTIONAL_LIGHT
#define MAX_DIRECTIONAL_LIGHT 2
#endif
#ifndef MAX_POINT_LIGHT
#define MAX_POINT_LIGHT 15
#endif

uniform int dirLtCount;
uniform int ptLtCount;
uniform DirectionalLight dirLights[MAX_DIRECTIONAL_LIGHT];
uniform PointLight ptLights[MAX_POINT_LIGHT];

#define EVAL_IBL
#ifndef MAX_IBL_LIGHT
#define MAX_IBL_LIGHT 1
#endif
uniform int iblCount;
struct IBL{
    samplerCube irradianceMap;
};
uniform IBL ibls[MAX_IBL_LIGHT];

/********************************************Config********************************************/
// Shadow map related variables
//#define SHADOW_HARD
//#define SHADOW_SOFT_ESM
//#define SHADOW_SOFT_PCF
#define SM_ESM_LOD 2
#define SM_PCF_FILTER 0.002

#define NUM_SAMPLES 10
#define NUM_RINGS 2
#define EPS 1e-5
#define MAX_BIAS 4.0
#define MIN_BIAS 0.4

#define EVAL_DIFFUSE
#define EVAL_SPECULAR

#ifndef SHADING_MODEL
#define SHADING_MODEL 1
#endif
/***************************************Shadow Making******************************************/
vec2 poissonDisk[NUM_SAMPLES];
highp float rand_2to1(vec2 uv ) {
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, M_PI );
	return fract(sin(sn) * c);
}
void poissonDiskSamples( const in vec2 randomSeed ) {
    float ANGLE_STEP = M_2PI * float( NUM_RINGS ) / float( NUM_SAMPLES );
    float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

    float angle = rand_2to1( randomSeed ) * M_2PI;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for( int i = 0; i < NUM_SAMPLES; i ++ ) {
        poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
        radius += radiusStep;
        angle += ANGLE_STEP;
    }
}
float PCF_Filter(sampler2D sm, vec2 uv, float zReceiver,
                 float filterRadius, float bias) {
    float sum = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        float depth = textureLod(sm, uv + poissonDisk[i] * filterRadius, 0).r;
        ATOMIC_COUNT_INCREMENT
        if (zReceiver <= depth + bias)
            sum += 1.0;
    }
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float depth = textureLod(sm, uv + -poissonDisk[i].yx * filterRadius, 0).r;
        ATOMIC_COUNT_INCREMENT
        if (zReceiver <= depth + bias)
            sum += 1.0;
    }
    return sum / (2.0 * float(NUM_SAMPLES));
}
float PCF_dir_visible(int index, vec3 position, float bias) {
    vec4 fromLight = dirLights[index].viewProj * vec4(position, 1.0);
    vec3 ndc = (fromLight.xyz / fromLight.w + 1.0) / 2.0;
    if(ndc.x <= 0.0 || ndc.x >= 1.0 || ndc.y <= 0.0 || ndc.y >= 1.0)
        return 1.0;
    vec2 uv = ndc.xy;
    float zReceiver = ndc.z; // Assumed to be eye-space z in this code
    float filterRadius = SM_PCF_FILTER;

    poissonDiskSamples(uv);
    return PCF_Filter(dirLights[index].shadowMap, uv, zReceiver, filterRadius, bias);
}
float PCF_pt_visible(int index, vec3 position, float bias) {
    vec4 fromLight = ptLights[index].viewProj * vec4(position, 1.0);
    vec3 ndc = (fromLight.xyz / fromLight.w + 1.0) / 2.0;
    if(ndc.x <= 0.0 || ndc.x >= 1.0 || ndc.y <= 0.0 || ndc.y >= 1.0)
        return 1.0;
    vec2 uv = ndc.xy;
    float zReceiver = ndc.z; // Assumed to be eye-space z in this code
    float filterRadius = SM_PCF_FILTER;

    poissonDiskSamples(uv);
    return PCF_Filter(ptLights[index].shadowMap, uv, zReceiver, filterRadius, bias);
}
float ESM_dir_visible(int index, vec3 position, float bias){
    vec4 fromLight = dirLights[index].viewProj * vec4(position, 1.0);
    vec3 ndc = (fromLight.xyz / fromLight.w + 1.0) / 2.0;
    if(ndc.x <= 0.0 || ndc.x >= 1.0 || ndc.y <= 0.0 || ndc.y >= 1.0)
        return 1.0;
    vec2 uv = ndc.xy;
    //ndc.z -= bias;
    float mindep = textureLod(dirLights[index].shadowMap, uv, SM_ESM_LOD).r;
    ATOMIC_COUNT_INCREMENT
    float sx = exp(-80.0 * ndc.z) * mindep;
    //return step(0.99, sx);
    return clamp(sx, 0.0, 1.0);
}
float ESM_pt_visible(int index, vec3 position, float bias){
    vec4 fromLight = ptLights[index].viewProj * vec4(position, 1.0);
    vec3 ndc = (fromLight.xyz / fromLight.w + 1.0) / 2.0;
    if(ndc.x <= 0.0 || ndc.x >= 1.0 || ndc.y <= 0.0 || ndc.y >= 1.0)
        return 1.0;
    vec2 uv = ndc.xy;
    //ndc.z -= bias;
    float mindep = textureLod(ptLights[index].shadowMap, uv, SM_ESM_LOD).r;
    ATOMIC_COUNT_INCREMENT
    float sx = exp(-80.0 * ndc.z) * mindep;
    //return step(0.99, sx);
    return pow(clamp(sx, 0.0, 1.0), 2.0);
}
float HARD_dir_visible(int index, vec3 position, float bias){
    vec4 fromLight = dirLights[index].viewProj * vec4(position, 1.0);
    vec3 ndc = (fromLight.xyz / fromLight.w + 1.0) / 2.0;
    if(ndc.x <= 0.0 || ndc.x >= 1.0 || ndc.y <= 0.0 || ndc.y >= 1.0)
        return 1.0;
    vec2 uv = ndc.xy;
    float mindep = texture(dirLights[index].shadowMap, uv).r;
    ATOMIC_COUNT_INCREMENT
    return step(ndc.z, mindep + bias);
}
float HARD_pt_visible(int index, vec3 position, float bias){
    vec4 fromLight = ptLights[index].viewProj * vec4(position, 1.0);
    vec3 ndc = (fromLight.xyz / fromLight.w + 1.0) / 2.0;
    if(ndc.x <= 0.0 || ndc.x >= 1.0 || ndc.y <= 0.0 || ndc.y >= 1.0)
        return 1.0;
    vec2 uv = ndc.xy;
    float mindep = texture(ptLights[index].shadowMap, uv).r;
    ATOMIC_COUNT_INCREMENT
    return step(ndc.z, mindep + bias);
}

/******************************************Lighting***********************************************/
vec3 evalColor(ShadingData sd, LightSample ls){
    vec3 diffuse = vec3(0.0), specular = vec3(0.0);
    #ifdef EVAL_DIFFUSE
    vec3 diffuseBrdf = evalDiffuseBrdf(sd, ls);
    diffuse = ls.color * diffuseBrdf * ls.NdotL;
    #endif
    #ifdef EVAL_SPECULAR
    vec3 specularBrdf = evalSpecularBrdf(sd, ls);
    specular = ls.color * specularBrdf * ls.NdotL;
    #endif
    return diffuse + specular;
}

vec3 evalDirLight(int index, ShadingData sd){
    LightSample ls;
    ls.L = -normalize(dirLights[index].direction);
    ls.NdotL = dot(sd.N, ls.L);
    float visibility;
    if(ls.NdotL <= 0.0)
        visibility = 0.0;
    else if(dirLights[index].has_shadow){
        float bias = max(MAX_BIAS * (1.0 - ls.NdotL), MIN_BIAS) * dirLights[index].resolution;
        #ifdef SHADOW_SOFT_PCF
        visibility = PCF_dir_visible(index, sd.posW, bias);
        #elif defined(SHADOW_SOFT_ESM)
        visibility = ESM_dir_visible(index, sd.posW, bias);
        #elif defined(SHADOW_HARD)
        visibility = HARD_dir_visible(index, sd.posW, bias);
        #else
        visibility = 1.0;
        #endif
    }else{
        visibility = 1.0;
    }
    vec3 H = normalize(sd.V + ls.L);
    ls.NdotH = dot(sd.N, H);
    ls.LdotH = dot(ls.L, H);
    ls.color = dirLights[index].intensity;
    vec3 ambient = dirLights[index].ambient * ls.color * sd.diffuse * sd.ao;
    if(visibility < EPS)
        return ambient;
    return visibility * evalColor(sd, ls) + ambient;
}

vec3 evalPtLight(int index, ShadingData sd){
    LightSample ls;
    ls.posW = ptLights[index].position;
    ls.L = normalize(ls.posW - sd.posW);
    ls.dist = length(ls.posW - sd.posW);
    float falloff = 1.0 / (ptLights[index].constant + ptLights[index].linear*ls.dist + ptLights[index].quadratic*ls.dist*ls.dist);
    float cosTheta = -dot(ls.L, normalize(ptLights[index].direction));
    if(ls.dist > ptLights[index].range || cosTheta < ptLights[index].cos_opening_angle)
        return vec3(0.0);
    else if(ptLights[index].penumbra_angle > 0.01){
        float deltaAngle = ptLights[index].opening_angle - acos(cosTheta);
        falloff *= clamp((deltaAngle - ptLights[index].penumbra_angle) / ptLights[index].penumbra_angle, 0.0, 1.0);
    }
    ls.NdotL = dot(sd.N, ls.L);
    float visibility;
    if(ls.NdotL <= 0.0)
        visibility = 0.0;
    else if(ptLights[index].has_shadow){
        float bias = max(MAX_BIAS * (1.0 - ls.NdotL), MIN_BIAS) * dirLights[index].resolution;
        #ifdef SHADOW_SOFT_PCF
        visibility = PCF_pt_visible(index, sd.posW, bias);
        #elif defined(SHADOW_SOFT_ESM)
        visibility = ESM_pt_visible(index, sd.posW, bias);
        #elif defined(SHADOW_HARD)
        visibility = HARD_pt_visible(index, sd.posW, bias);
        #else
        visibility = 1.0;
        #endif
    }else{
        visibility = 1.0;
    }
    vec3 H = normalize(sd.V + ls.L);
    ls.NdotH = dot(sd.N, H);
    ls.LdotH = dot(ls.L, H);
    ls.color = ptLights[index].intensity * falloff;
    vec3 ambient = ptLights[index].ambient * ls.color * sd.diffuse * sd.ao;
    if(visibility < EPS)
        return ambient;
    return visibility * evalColor(sd, ls) + ambient;
}

vec3 evalIBL(int index, ShadingData sd){
    LightSample ls;
    ls.color = texture(ibls[index].irradianceMap, sd.N).rgb;
    ATOMIC_COUNT_INCREMENT
    vec3 kS = fresnelSchlick(sd.specular, max(vec3(1.0 - sd.linearRoughness), sd.specular), max(sd.NdotV, 0.0));
    //vec3 kS = evalSpecularBrdf(sd, ls);
    vec3 kD = 1.0 - kS;
    vec3 diffuse = ls.color * sd.diffuse * kD * sd.ao * M_1_PI;
    return diffuse;
}

vec3 evalShading(vec3 baseColor, vec3 specColor, vec3 normal, vec4 position, float ao){
    ShadingData sd;
    sd.posW = position.xyz;
    sd.V = normalize(camera_pos - sd.posW);
    sd.N = normal;
    sd.NdotV = dot(sd.V, sd.N);
    sd.lineardep = position.w;
    sd.ao = 1.0 - ao;

    #if SHADING_MODEL == 1
    // Shading Model Metal Rough
    float IoR = 1.5;
    float f = (IoR - 1.0) / (IoR + 1.0);
    vec3 F0 = vec3(f * f);
    sd.diffuse = mix(baseColor.rgb, vec3(0), specColor.b);
    sd.metallic = specColor.b;
    sd.specular = mix(F0, baseColor.rgb, specColor.b);
    sd.linearRoughness = specColor.g;
    sd.ggxAlpha = max(0.0064, sd.linearRoughness * sd.linearRoughness);
    #elif SHADING_MODEL == 2
    sd.diffuse = baseColor.rgb;
    sd.specular = specColor.rgb;
    sd.linearRoughness = 0.0;
    sd.ggxAlpha = 0.0;
    #endif

    vec3 color = vec3(0.0, 0.0, 0.0);    
    int count = min(dirLtCount, MAX_DIRECTIONAL_LIGHT);
    for (int i = 0; i < count; i++){
        color += evalDirLight(i, sd);
    }
    count = min(ptLtCount, MAX_POINT_LIGHT);
    for (int i = 0; i < count; i++){
        color += evalPtLight(i, sd); 
    }
    #ifdef EVAL_IBL
    count = min(iblCount, MAX_IBL_LIGHT);
    for (int i = 0; i < count; i++){
        color += evalIBL(i, sd); 
    }
    #endif
    return color;
}
//sd.metallic = specColor.b;
//sd.linearRoughness = 1 - specColor.a;
//sd.ggxAlpha = max(0.0064, specColor.g * specColor.g);