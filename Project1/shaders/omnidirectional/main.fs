out vec4 FragColor;

in vec4 FragPos;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
}