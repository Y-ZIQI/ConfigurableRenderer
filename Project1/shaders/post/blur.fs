//++`shaders/shading/defines.glsl`
//++`shaders/shading/gaussian.glsl`

out vec3 outColor;

in vec2 TexCoords;

uniform vec2 pixel_size;
uniform bool fixed_kernel;
uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;

//#define COMPLEX_BLOOM

int kernel(float L){
    return int(10.0 * min(log(L + 1.5), 2.99));
}

void main()
{
#ifdef COMPLEX_BLOOM
    const int ksize = 30;
    const vec3 Lumia = vec3(0.2126, 0.7152, 0.0722);

    vec3 result;
    if(horizontal){
        vec3 color = textureLod(colorTex, TexCoords, 1).rgb;
        float L = dot(color, Lumia);
        int offset = kernel(L);
        result = color * gaussian_weights[gaussian_parts[offset]];
        for(int i = 1;i < ksize;i++){
            vec3 colorLeft = textureLod(colorTex, TexCoords + vec2(-i, 0) * pixel_size, 1).rgb;
            int offsetL = kernel(dot(colorLeft, Lumia));
            if(i < offsetL)
                result += colorLeft * gaussian_weights[gaussian_parts[offsetL] + i];
            vec3 colorRight = textureLod(colorTex, TexCoords + vec2(i, 0) * pixel_size, 1).rgb;
            int offsetR = kernel(dot(colorRight, Lumia));
            if(i < offsetR)
                result += colorRight * gaussian_weights[gaussian_parts[offsetR] + i];
        }
    }else{
        vec3 color = texture(horizontalTex, TexCoords).rgb;
        float L = dot(color, Lumia);
        int offset = kernel(L);
        result = color * gaussian_weights[gaussian_parts[offset]];
        for(int i = 1;i < ksize;i++){
            vec3 colorBottom = texture(horizontalTex, TexCoords + vec2(0, -i) * pixel_size).rgb;
            int offsetB = kernel(dot(colorBottom, Lumia));
            if(i < offsetB)
                result += colorBottom * gaussian_weights[gaussian_parts[offsetB] + i];
            vec3 colorTop = texture(horizontalTex, TexCoords + vec2(0, i) * pixel_size).rgb;
            int offsetT = kernel(dot(colorTop, Lumia));
            if(i < offsetT)
                result += colorTop * gaussian_weights[gaussian_parts[offsetT] + i];
        }
    }
    outColor = result;
    ATOMIC_COUNT_INCREMENTS(ksize * 2 - 1)    
    ATOMIC_COUNT_CALCULATE
#else
    const int ksize = 20;
    vec3 result;
    int start = gaussian_parts[ksize - 1];
    if(horizontal){
        result = textureLod(colorTex, TexCoords, 1).rgb * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureLod(colorTex, TexCoords + vec2(i, 0) * pixel_size, 1).rgb * gaussian_weights[start + i];
            result += textureLod(colorTex, TexCoords + vec2(-i, 0) * pixel_size, 1).rgb * gaussian_weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords).rgb * gaussian_weights[start];
        for(int i = 1;i < ksize;i++){
            result += texture(horizontalTex, TexCoords + vec2(0, i) * pixel_size).rgb * gaussian_weights[start + i];
            result += texture(horizontalTex, TexCoords + vec2(0, -i) * pixel_size).rgb * gaussian_weights[start + i];
        }
    }
    outColor = result;
    ATOMIC_COUNT_INCREMENTS(ksize * 2 - 1)    
    ATOMIC_COUNT_CALCULATE
#endif
}
