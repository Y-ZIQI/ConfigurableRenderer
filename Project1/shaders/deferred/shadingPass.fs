//++`shaders/shading/shading.glsl`

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
    ATOMIC_COUNT_INCREMENT
    if(dot(normal, normal) == 0.0){
        FragColor = texture(albedoTex, TexCoords).rgb;
        BloomColor = vec3(0.0);
        ATOMIC_COUNT_INCREMENT
    }else{
        vec3 diffuse = texture(albedoTex, TexCoords).rgb;
        ATOMIC_COUNT_INCREMENT
        vec3 specular = texture(specularTex, TexCoords).rgb;
        ATOMIC_COUNT_INCREMENT
        vec3 emissive = texture(emissiveTex, TexCoords).rgb;
        ATOMIC_COUNT_INCREMENT
        vec3 position = texture(positionTex, TexCoords).rgb;
        ATOMIC_COUNT_INCREMENT
        float lineardepth = texture(depthTex, TexCoords).r;
        ATOMIC_COUNT_INCREMENT
        #ifdef SSAO
        float ao = 0.0;
        for(int x = -2;x<=2;x++){
            for(int y = -2;y<=2;y++){
                ao += textureOffset(aoTex, TexCoords, ivec2(x, y)).r;
                ATOMIC_COUNT_INCREMENT
            }
        }
        ao /= 25.0;
        #else
        float ao = 0.0;
        #endif

        /* `evalShading` is in shading/shading.glsl */
        setRandomRotate(TexCoords);
        vec3 color = evalShading(diffuse, specular, emissive, normal, position, lineardepth, ao);

        /*const float gamma = 2.2;
        const float exposure = 1.0;
        vec3 mapped = pow(color, vec3(1.0 / gamma));*/

        FragColor = color;
        const vec3 Lumia = vec3(0.2126, 0.7152, 0.0722);
        if(dot(color, Lumia) >= 1.0){
            BloomColor = max(color, emissive);
        }else
            BloomColor = emissive;
        
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
//FragColor = vec4(vec3(1.0 - ao), 1.0);
//FragColor = vec4(normal, 1.0);
/*if(TexCoords.x<0.5 && TexCoords.y<0.5){
    FragColor = sqrt(vec4(texture(dirLights[0].shadowMap, TexCoords * 2.0).rrr, 1.0));
}*/
//vec3 mapped = color / (color + vec3(1.0));
//vec3 mapped = 1.0 - exp(-color * exposure);
//mapped = pow(mapped, vec3(1.0 / gamma));