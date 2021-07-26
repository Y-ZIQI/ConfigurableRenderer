//++`shaders/shading/defines.glsl`
//++`shaders/shading/gaussian.glsl`

out vec3 outColor;

in vec2 TexCoords;

uniform bool fixed_kernel;
uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;

const float weight[] = {
    0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
};

const int ksize = 3;

void main()
{
    //const vec3 Lumia = vec3(0.2126, 0.7152, 0.0722);
    //float L = dot(color, Lumia);
    //int kSize = int(log(L));
    vec3 result;
    int start = gaussian_parts[ksize - 1];
    if(horizontal){
        vec3 color = texture(colorTex, TexCoords).rgb;
        result = color * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(colorTex, TexCoords, ivec2(i, 0)).rgb * gaussian_weights[start + i];
            result += textureOffset(colorTex, TexCoords, ivec2(-i, 0)).rgb * gaussian_weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords).rgb * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, i)).rgb * gaussian_weights[start + i];
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, -i)).rgb * gaussian_weights[start + i];
        }
    }
    outColor = result;
}