layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat3 normal_model;
uniform mat4 model;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = model * vec4(position, 1.0);
}