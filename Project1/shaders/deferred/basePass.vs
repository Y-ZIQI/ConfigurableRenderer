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
uniform mat4 camera_vp;

void main()
{
    TBN = normal_model * mat3(aTangent, aBitangent, aNormal);
    TexCoords = aTexCoords;
    vec4 _WorldPos = model * vec4(aPos, 1.0);
    WorldPos = _WorldPos.xyz / _WorldPos.w;
    gl_Position = camera_vp * vec4(WorldPos, 1.0);
}