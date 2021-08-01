//++`shaders/shading/defines.glsl`
//++`shaders/shading/gaussian.glsl`

out vec4 outColor;

in vec3 TexCoords;
in vec4 FragPos;

uniform int ksize;

uniform vec2 pixel_size;
uniform bool horizontal;
uniform samplerCube colorTex;
uniform samplerCube horizontalTex;

vec3 dirOffset(vec3 dir, vec2 uv){
    vec3 absdir = abs(dir);
    if(absdir.x > absdir.y)
        if(absdir.x > absdir.z) return dir + vec3(0.0, uv);
        else return dir + vec3(uv, 0.0);
    else
        if(absdir.y > absdir.z) return dir + vec3(uv.x, 0.0, uv.y);
        else return dir + vec3(uv, 0.0);
}

void main()
{
    vec4 result;
    int start = gaussian_parts[ksize - 1];
    if(horizontal){
        vec4 color = texture(colorTex, TexCoords);
        result = color * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += texture(colorTex, dirOffset(TexCoords, vec2(i, 0) * pixel_size)) * gaussian_weights[start + i];
            result += texture(colorTex, dirOffset(TexCoords, vec2(-i, 0) * pixel_size)) * gaussian_weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords) * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += texture(horizontalTex, dirOffset(TexCoords, vec2(0, i) * pixel_size)) * gaussian_weights[start + i];
            result += texture(horizontalTex, dirOffset(TexCoords, vec2(0, -i) * pixel_size)) * gaussian_weights[start + i];
        }
    }
    outColor = result;
    ATOMIC_COUNT_INCREMENTS(ksize * 2 - 1)    
    ATOMIC_COUNT_CALCULATE
}