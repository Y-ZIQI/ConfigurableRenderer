layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    mat3 vTBN;
    vec2 vTexCoords;
    vec3 vWorldPos;
} vs_out;

uniform mat3 normal_model;
uniform mat4 model;

void main()
{
    vs_out.vTBN = normal_model * mat3(aTangent, aBitangent, aNormal);
    vs_out.vTBN[0] = normalize(vs_out.vTBN[0]);
    vs_out.vTBN[1] = normalize(vs_out.vTBN[1]);
    vs_out.vTBN[2] = normalize(vs_out.vTBN[2]);
    vs_out.vTexCoords = aTexCoords;
    vec4 _WorldPos = model * vec4(aPos, 1.0);
    vs_out.vWorldPos = _WorldPos.xyz / _WorldPos.w;
    gl_Position = vec4(vs_out.vWorldPos, 1.0);
}

/*layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 avTexCoords;

out vec2 vTexCoords;

uniform mat3 normal_model;
uniform mat4 model;

void main()
{
    vTexCoords = avTexCoords;
    gl_Position = model * vec4(position, 1.0);
}*/