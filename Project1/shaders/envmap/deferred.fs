//++`shaders/shading/defines.glsl`

layout (location = 2) out vec4 fAlbedo;

in vec3 TexCoords;

uniform samplerCube envmap;

void main()
{    
    fAlbedo = texture(envmap, TexCoords);
    ATOMIC_COUNTER_I_INCREMENT(0)
}