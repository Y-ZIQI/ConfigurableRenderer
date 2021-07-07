//++`shaders/shading/shading.glsl`

//#define SSAO
//#define SHOW_GRID

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D albedoTex;
uniform sampler2D specularTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;

#ifdef SSAO
uniform sampler2D aoTex;
#endif

void main()
{
    vec3 normal = texture(normalTex, TexCoords).rgb;
    ATOMIC_COUNT_INCREMENT
    if(dot(normal, normal) == 0.0){
        FragColor = vec4(texture(albedoTex, TexCoords).rgb, 1.0);
        ATOMIC_COUNT_INCREMENT
    }else{
        vec4 diffuse = texture(albedoTex, TexCoords);
        ATOMIC_COUNT_INCREMENT
        vec3 specular = texture(specularTex, TexCoords).rgb;
        ATOMIC_COUNT_INCREMENT
        vec4 position = texture(positionTex, TexCoords);
        ATOMIC_COUNT_INCREMENT
        #ifdef SSAO
        //float ao = texture(aoTex, TexCoords).r;
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
        vec3 color = evalShading(diffuse.rgb, specular, normal, position, ao);

        const float gamma = 2.2;
        const float exposure = 1.0;
        vec3 mapped = pow(color, vec3(1.0 / gamma));

        FragColor = vec4(mapped, 1.0);
        
        #ifdef SHOW_GRID
        if(fract(position.x) < 0.004)
            FragColor = vec4(sign(position.x), 0.0, -sign(position.x), 1.0);
        else if(fract(position.z) < 0.004)
            FragColor = vec4(sign(position.z), 0.0, -sign(position.z), 1.0);
        if(abs(position.x) < 0.005 || abs(position.z) < 0.005)
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);
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