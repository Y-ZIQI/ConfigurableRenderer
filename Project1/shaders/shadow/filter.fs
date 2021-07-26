//++`shaders/shading/defines.glsl`
//++`shaders/shading/gaussian.glsl`

out vec4 outColor;

in vec2 TexCoords;

uniform int ksize;

uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;

void main()
{
    vec4 result;
    int start = gaussian_parts[ksize - 1];
    if(horizontal){
        vec4 color = texture(colorTex, TexCoords);
        result = color * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(colorTex, TexCoords, ivec2(i, 0)) * gaussian_weights[start + i];
            result += textureOffset(colorTex, TexCoords, ivec2(-i, 0)) * gaussian_weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords) * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, i)) * gaussian_weights[start + i];
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, -i)) * gaussian_weights[start + i];
        }
    }
    outColor = result;
}