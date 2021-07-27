//++`shaders/shading/defines.glsl`
//++`shaders/shading/shading.glsl`

#ifdef OPENGL_SAMPLER
#define sampleTexture(a, b) texture(a, b)
#else
#define sampleTexture(a, b) texture(a, vec2(b.x, 1.0 - b.y))
#endif

#define BASECOLOR_BIT   0x1
#define SPECULAR_BIT    0x2
#define NORMAL_BIT      0x4
#define EMISSIVE_BIT    0x8

out vec3 FragColor;

in mat3 TBN;
in vec2 TexCoords;
in vec3 WorldPos;
in vec4 FragPos;

uniform int has_tex;
uniform vec4 const_color;
uniform vec3 const_specular;
uniform vec3 const_emissive;

uniform sampler2D baseColorMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;
uniform sampler2D emissiveMap;

void main()
{
    vec4 albedo;
    if((has_tex & BASECOLOR_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        albedo = sampleTexture(baseColorMap, TexCoords);
    }else
        albedo = const_color;
    float alpha = albedo.a;
    if(alpha <= 0.5){
        discard;
    }
    vec3 specular;
    if((has_tex & SPECULAR_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        specular = sampleTexture(specularMap, TexCoords).rgb;
    }else
        specular = const_specular;
    vec3 emissive;
    if((has_tex & EMISSIVE_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        emissive = sampleTexture(emissiveMap, TexCoords).rgb;
    }else
        emissive = const_emissive;
    vec3 normal;
    if((has_tex & NORMAL_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        normal = vec3(sampleTexture(normalMap, TexCoords).rg, 0.0);
        normal.z = sqrt(1.0 - clamp(dot(normal, normal), 0.0, 1.0));
        normal.xy = normal.xy * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }else
        normal = TBN[2];

    vec3 color = evalShading(albedo.rgb, specular, emissive * M_PI, normal, WorldPos, 1.0, 0.0);
    
    //const float gamma = 2.2;
    //vec3 mapped = pow(color, vec3(1.0 / gamma));
    FragColor = color;
    ATOMIC_COUNT_CALCULATE
}

/*out vec4 FragColor;

in vec4 FragPos;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    ATOMIC_COUNTER_I_INCREMENT(0)
    FragColor = texture(texture_diffuse1, TexCoords);
}*/