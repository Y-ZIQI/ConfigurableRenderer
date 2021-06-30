//++`shaders/shading/defines.glsl`

layout (location = 2) out vec4 fAlbedo;

in vec3 TexCoords;

uniform samplerCube envmap;

void main()
{    
    fAlbedo.rgb = texture(envmap, TexCoords).rgb;
    ATOMIC_COUNT_INCREMENT
}