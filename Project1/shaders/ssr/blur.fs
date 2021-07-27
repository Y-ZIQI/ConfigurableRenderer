//++`shaders/shading/defines.glsl`
//++`shaders/shading/gaussian.glsl`

out vec3 outColor;

in vec2 TexCoords;

uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;
uniform sampler2D specularTex;

void main()
{
    vec3 specular = texture(specularTex, TexCoords).rgb;
    float roughness = specular.g;
    int kSize = int(min(sqrt(roughness), 0.99) * (MAX_KERNEL_SIZE - 1)) + 1;

    vec3 result;
    int start = gaussian_parts[kSize];
    if(horizontal){
        vec3 color = texture(colorTex, TexCoords).rgb;
        result = color * gaussian_weights[start];
        for(int i = 1;i <= kSize;i++){
            result += textureOffset(colorTex, TexCoords, ivec2(i, 0)).rgb * gaussian_weights[start + i];
            result += textureOffset(colorTex, TexCoords, ivec2(-i, 0)).rgb * gaussian_weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords).rgb * gaussian_weights[start];
        for(int i = 1;i <= kSize;i++){
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, i)).rgb * gaussian_weights[start + i];
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, -i)).rgb * gaussian_weights[start + i];
        }
    }
    outColor = result;
    ATOMIC_COUNT_INCREMENTS(kSize * 2 + 2)    
    ATOMIC_COUNT_CALCULATE
}