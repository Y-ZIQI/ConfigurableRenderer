//++`shaders/shading/defines.glsl`
//++`shaders/shading/alphaTest.glsl`

//#define OPENGL_SAMPLER
#ifdef OPENGL_SAMPLER
#define sampleTexture(a, b) texture(a, b)
#else
#define sampleTexture(a, b) texture(a, vec2(b.x, 1.0 - b.y))
#endif

#define USE_NORMALMAP true
#define BASECOLOR_BIT   0x1
#define SPECULAR_BIT    0x2
#define NORMAL_BIT      0x4
#define EMISSIVE_BIT    0x8

layout (location = 0) out vec4 fAlbedo;
layout (location = 1) out vec4 fSpecular;
layout (location = 2) out vec4 fPosition;
layout (location = 3) out vec4 fNormal;
layout (location = 4) out vec4 fEmissive;

in mat3 TBN;
in vec2 TexCoords;
in vec3 WorldPos;

uniform vec3 camera_pos;
uniform vec4 camera_params;

uniform int has_tex;
uniform vec4 const_color;
uniform vec4 const_specular;
uniform vec4 const_emissive;

uniform sampler2D baseColorMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;
uniform sampler2D emissiveMap;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return 2.0 / (camera_params.z - z * camera_params.w);    
}

void main()
{
    if((has_tex & BASECOLOR_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        fAlbedo = sampleTexture(baseColorMap, TexCoords);
    }else
        fAlbedo = const_color;
    float alpha = fAlbedo.a;
    if(evalAlphaTest(alpha, 0.5, vec3(0.0))){
        discard;
    }
    if((has_tex & SPECULAR_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        fSpecular = sampleTexture(specularMap, TexCoords);
    }else
        fSpecular = const_specular;
    fPosition = vec4(WorldPos.xyz, LinearizeDepth(gl_FragCoord.z));
    if((has_tex & NORMAL_BIT) != 0x0 && USE_NORMALMAP){
        ATOMIC_COUNT_INCREMENT
        vec3 normal = vec3(sampleTexture(normalMap, TexCoords).rg, 0.0);
        normal.z = sqrt(1.0 - clamp(dot(normal, normal), 0.0, 1.0));
        normal.xy = normal.xy * 2.0 - 1.0;
        fNormal = vec4(normalize(TBN * normal), 1.0);
    }else
        fNormal = vec4(normalize(TBN[2]), 1.0);
    if((has_tex & EMISSIVE_BIT) != 0x0){
        ATOMIC_COUNT_INCREMENT
        fEmissive = sampleTexture(emissiveMap, TexCoords);
    }else
        fEmissive = const_emissive;

    /*if(tex_based){
        ATOMIC_COUNT_INCREMENT
        fAlbedo = sampleTexture(baseColorMap, TexCoords);
        ATOMIC_COUNT_INCREMENT
        fSpecular = vec4(sampleTexture(specularMap, TexCoords).rgb, fAlbedo.a);
    }else{
        fAlbedo = const_color;
        fSpecular = const_specular;
    }
    float alpha = fAlbedo.a;
    if(evalAlphaTest(alpha, 0.5, vec3(0.0))){
        discard;
    }
    fPosition = vec4(WorldPos.xyz, LinearizeDepth(gl_FragCoord.z));
    if(has_normalmap && USE_NORMALMAP){
        ATOMIC_COUNT_INCREMENT
        vec3 normal = vec3(sampleTexture(normalMap, TexCoords).rg, 0.0);
        normal.z = sqrt(1.0 - clamp(dot(normal, normal), 0.0, 1.0));
        normal.xy = normal.xy * 2.0 - 1.0;
        fNormal = vec4(normalize(TBN * normal), alpha);
    }else{
        fNormal = vec4(normalize(TBN[2]), alpha);
    }*/
    ATOMIC_COUNT_CALCULATE
}