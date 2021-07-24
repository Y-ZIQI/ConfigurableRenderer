//++`shaders/shading/defines.glsl`

out vec3 outColor;

in vec2 TexCoords;

uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;

const float weight[] = {
    0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
};

void main()
{
    vec3 color = texture(colorTex, TexCoords).rgb;
    const vec3 Lumia = vec3(0.2126, 0.7152, 0.0722);
    float L = dot(color, Lumia);
    int kSize = int(log(L));
    vec3 result;
    if(horizontal){
        result = color * weight[0];
        for(int i = 1;i < 5;i++){
            result += textureOffset(colorTex, TexCoords, ivec2(i, 0)).rgb * weight[i];
            result += textureOffset(colorTex, TexCoords, ivec2(-i, 0)).rgb * weight[i];
        }
    }else{
        result = texture(horizontalTex, TexCoords).rgb * weight[0];
        for(int i = 1;i < 5;i++){
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, i)).rgb * weight[i];
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, -i)).rgb * weight[i];
        }
    }
    outColor = result;
}