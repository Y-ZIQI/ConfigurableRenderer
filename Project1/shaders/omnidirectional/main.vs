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

/*
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out mat3 TBN;
out vec2 TexCoords;
out vec3 WorldPos;

uniform mat3 normal_model;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TBN = normal_model * mat3(aTangent, aBitangent, aNormal);
    TexCoords = aTexCoords;
    vec4 _WorldPos = model * vec4(aPos, 1.0);
    WorldPos = _WorldPos.xyz / _WorldPos.w;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
*/