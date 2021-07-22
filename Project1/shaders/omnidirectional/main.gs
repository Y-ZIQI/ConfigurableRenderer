layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

in VS_OUT {
    mat3 vTBN;
    vec2 vTexCoords;
    vec3 vWorldPos;
} gs_in[];

uniform mat4 transforms[6];

out mat3 TBN;
out vec2 TexCoords;
out vec3 WorldPos;
out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle's vertices
        {
            TBN = gs_in[i].vTBN;
            TexCoords = gs_in[i].vTexCoords;
            WorldPos = gs_in[i].vWorldPos;
            FragPos = gl_in[i].gl_Position;
            gl_Position = transforms[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
}