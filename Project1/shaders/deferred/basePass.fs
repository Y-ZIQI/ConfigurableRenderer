//++`shaders/shading/defines.glsl`

//#define OPENGL_SAMPLER
#ifdef OPENGL_SAMPLER
#define sampleTexture(a, b) texture(a, b)
#else
#define sampleTexture(a, b) texture(a, vec2(b.x, 1.0 - b.y))
#endif

#define USE_SHADOWMAP true
#define BASECOLOR_BIT   0x1
#define SPECULAR_BIT    0x2
#define NORMAL_BIT      0x4
#define EMISSIVE_BIT    0x8

layout (location = 0) out vec4 fAlbedo;
layout (location = 1) out vec3 fSpecular;
layout (location = 2) out vec3 fEmissive;
layout (location = 3) out vec3 fPosition;
layout (location = 4) out vec4 fTangent;
layout (location = 5) out vec4 fNormal;

in mat3 TBN;
in vec2 TexCoords;
in vec3 WorldPos;

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
    if((has_tex & BASECOLOR_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        fAlbedo = sampleTexture(baseColorMap, TexCoords);
        //fAlbedo = vec4(1.0, 1.0, 1.0, 1.0);
    }else
        fAlbedo = const_color;
    float alpha = fAlbedo.a;
    if(alpha <= 0.5){
        discard;
    }
    if((has_tex & SPECULAR_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        fSpecular = sampleTexture(specularMap, TexCoords).rgb;
        //fSpecular = vec3(0.0, 1.0, 0.0);
    }else
        fSpecular = const_specular;
    if((has_tex & EMISSIVE_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        fEmissive = sampleTexture(emissiveMap, TexCoords).rgb;
    }else
        fEmissive = const_emissive;
    fPosition = WorldPos;
    if((has_tex & NORMAL_BIT) != 0x0 && USE_SHADOWMAP){
        ATOMIC_COUNT_INCREMENT
        vec2 normal_rg = sampleTexture(normalMap, TexCoords).rg;
        fTangent = vec4(TBN[0], normal_rg.r);
        fNormal = vec4(TBN[2], normal_rg.g);
    }else{
        fTangent = vec4(TBN[0], 0.5);
        fNormal = vec4(TBN[2], 0.5);
    }
    ATOMIC_COUNT_CALCULATE
}