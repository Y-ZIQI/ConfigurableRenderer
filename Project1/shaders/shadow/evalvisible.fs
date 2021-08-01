//++`shaders/shading/defines.glsl`

out float visible;

in vec2 TexCoords;

#define EPS 1e-5
#define MAX_BIAS 2.0
#define MIN_BIAS 0.2

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform sampler2D positionTex;
uniform sampler2D normalTex;

struct DirectionalLight{
    vec3 intensity;
    vec3 direction;
    float ambient;

    bool has_shadow;
    mat4 viewProj;
    sampler2D shadowMap;
    float resolution;
    float light_size;
};
uniform DirectionalLight dirLight;

float HARD_dir_visible(vec3 position, float bias){
    vec3 ndc;
    if(!posFromLight(dirLight.viewProj, position, ndc)) return 1.0;
    float mindep = texture(dirLight.shadowMap, ndc.xy).r;
    ATOMIC_COUNT_INCREMENT
    return step(ndc.z, mindep + bias);
}

void main()
{
    float position = texture(positionTex, TexCoords).rgb;
    vec3 normal = texture(normalTex, TexCoords).rgb;
    vec3 L = normalize(dirLight.position - position);
    float NdotL = dot(normal, L);
    if(NdotL < 0.0) visible = 0.0;
    float bias = mix(MAX_BIAS, MIN_BIAS, NdotL) / dirLight.resolution;
    visible = HARD_dir_visible(position, bias);
    ATOMIC_COUNT_CALCULATE
}