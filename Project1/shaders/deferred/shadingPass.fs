//++`shaders/shading/shading.glsl`

//#define BLOOM
//#define SSAO
//#define SHOW_GRID

layout (location = 0) out vec3 FragColor;
layout (location = 1) out vec3 BloomColor;

in vec2 TexCoords;

uniform sampler2D albedoTex;
uniform sampler2D specularTex;
uniform sampler2D emissiveTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;
uniform sampler2D aoTex;

void main()
{
    vec3 normal = texture(normalTex, TexCoords).rgb;
    if(dot(normal, normal) == 0.0){
        ATOMIC_COUNT_INCREMENTS(2)
        FragColor = texture(albedoTex, TexCoords).rgb;
        BloomColor = vec3(0.0);
    }else{
        vec3 diffuse = texture(albedoTex, TexCoords).rgb;
        vec3 specular = texture(specularTex, TexCoords).rgb;
        vec3 emissive = texture(emissiveTex, TexCoords).rgb * M_PI;
        vec3 position = texture(positionTex, TexCoords).rgb;
        float lineardepth = texture(depthTex, TexCoords).r;

        #ifdef SSAO
        ATOMIC_COUNT_INCREMENTS(7)
        float ao = texture(aoTex, TexCoords).r;
        #else
        ATOMIC_COUNT_INCREMENTS(6)
        float ao = 0.0;
        #endif

        /* `evalShading` is in shading/shading.glsl */
        setRandomRotate(TexCoords);
        vec3 color = evalShading(diffuse, specular, emissive, normal, position, lineardepth, ao);

        
        #ifdef BLOOM
        const float bloom_ratio = 0.3;
        const vec3 Lumia = vec3(0.2126, 0.7152, 0.0722);
        float rate = dot(color, Lumia);
        vec3 clip_color;
        if(rate > 1.0){
            clip_color = (color - color / rate) * bloom_ratio;
        }else{
            clip_color = emissive * bloom_ratio;
        }
        BloomColor = 25.0 * log(0.04 * clip_color + 1.0);
        FragColor = color - clip_color;
        #else
        BloomColor = vec3(0.0);
        FragColor = color;
        #endif
        
        #ifdef SHOW_GRID
        if(fract(position.x) < 0.004)
            FragColor = vec3(sign(position.x), 0.0, -sign(position.x));
        else if(fract(position.z) < 0.004)
            FragColor = vec3(sign(position.z), 0.0, -sign(position.z));
        if(abs(position.x) < 0.005 || abs(position.z) < 0.005)
            FragColor = vec3(0.0, 1.0, 0.0);
        #endif
    }
    ATOMIC_COUNT_CALCULATE
}