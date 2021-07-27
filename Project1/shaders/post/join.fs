//++`shaders/shading/defines.glsl`

out vec3 outColor;

in vec2 TexCoords;

uniform bool join;
uniform bool substract;
uniform bool tone_mapping;
uniform sampler2D colorTex;
uniform sampler2D joinTex;
uniform sampler2D substractTex;

void main()
{
    vec3 hdrColor = vec3(0.0);
    ATOMIC_COUNT_INCREMENT
    hdrColor += texture(colorTex, TexCoords).rgb;
    if(join) {
        ATOMIC_COUNT_INCREMENT
        hdrColor += texture(joinTex, TexCoords).rgb;
    }
    if(substract){
        ATOMIC_COUNT_INCREMENT
        hdrColor -= texture(substractTex, TexCoords).rgb;
    }
    
    if(tone_mapping){
        const float gamma = 2.2;
        const float exposure = 1.0;
        //vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
        vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
        mapped = pow(mapped, vec3(1.0 / gamma));
        outColor = mapped;
    }else
        outColor = hdrColor;
    ATOMIC_COUNT_CALCULATE
}