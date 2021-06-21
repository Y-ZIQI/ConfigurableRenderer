out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTex;

void main()
{
    FragColor = texture(screenTex, TexCoords).rgba;
}