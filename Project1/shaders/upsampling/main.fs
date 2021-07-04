//++`shaders/shading/defines.glsl`

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTex;

void main()
{
    ATOMIC_COUNTER_I_INCREMENT(0)
    FragColor = texture(screenTex, TexCoords).rgba;
}