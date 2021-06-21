out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube envmap;

void main()
{    
    FragColor = texture(envmap, TexCoords);
}