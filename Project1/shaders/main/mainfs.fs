//++`shaders/shading/shading.glsl`
//++`shaders/shading/alphaTest.glsl`

#ifdef OPENGL_SAMPLER
#define sampleTexture(a, b) texture(a, b)
#else
#define sampleTexture(a, b) texture(a, vec2(b.x, 1.0 - b.y))
#endif

out vec4 FragColor;

in mat3 TBN;
in vec2 TexCoords;
in vec3 WorldPos;

uniform bool has_normalmap;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

void main()
{
    vec4 diffColor = sampleTexture(texture_diffuse1, TexCoords);
    float alpha = diffColor.a;
    if(evalAlphaTest(alpha, 0.5, vec3(0.0))){
        discard;
    }
    vec4 specColor = sampleTexture(texture_specular1, TexCoords);
    vec3 normal;
    if(has_normalmap){
        //normal = sampleTexture(texture_normal1, TexCoords).rgb;
        normal = vec3(sampleTexture(texture_normal1, TexCoords).rg, 0.0);
        normal.z = sqrt(1.0 - dot(normal, normal));

        normal = normal * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }else{
        normal = normalize(TBN[2]);
    }

    /* `evalShading` is in shading/shading.glsl */
    vec3 color = evalShading(diffColor.rgb, specColor.rgb, normal, WorldPos);
    const float exposure = 1.0;
    vec3 mapped = 1.0 - exp(-color * exposure);
    FragColor = vec4(mapped, alpha);
}