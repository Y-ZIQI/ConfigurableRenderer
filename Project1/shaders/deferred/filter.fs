//++`shaders/shading/defines.glsl`
//++`shaders/shading/gaussian.glsl`

out float outColor;

in vec2 TexCoords;

uniform int ksize;

uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;

void main()
{
    float result;
    int start = gaussian_parts[ksize - 1];
    if(horizontal){
        float color = texture(colorTex, TexCoords).r;
        result = color * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(colorTex, TexCoords, ivec2(i, 0)).r * gaussian_weights[start + i];
            result += textureOffset(colorTex, TexCoords, ivec2(-i, 0)).r * gaussian_weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords).r * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, i)).r * gaussian_weights[start + i];
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, -i)).r * gaussian_weights[start + i];
        }
    }
    outColor = result;
    ATOMIC_COUNT_INCREMENTS(ksize * 2 - 1)    
    ATOMIC_COUNT_CALCULATE
}