layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 transforms[6];

in vec3 vTexCoords[];

out vec3 TexCoords;
out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle's vertices
        {
            TexCoords = vTexCoords[i];
            FragPos = transforms[face] * gl_in[i].gl_Position;
            gl_Position = FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
}