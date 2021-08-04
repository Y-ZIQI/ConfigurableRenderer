//++`shaders/shading/defines.glsl`

out vec3 outColor;

in vec2 TexCoords;

uniform bool join;
uniform bool substract;
uniform bool tone_mapping;
uniform sampler2D colorTex;
uniform sampler2D joinTex;

void main()
{
    vec3 hdrColor = vec3(0.0);
    if(join) {
        hdrColor += texture(colorTex, TexCoords).rgb;
        if(substract) hdrColor += texture(joinTex, TexCoords).rgb * 0.3;
        else hdrColor += texture(joinTex, TexCoords).rgb;
        ATOMIC_COUNTER_I_INCREMENT(1)
    }else{
        hdrColor += texture(colorTex, TexCoords).rgb;
        ATOMIC_COUNTER_I_INCREMENT(0)
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
}