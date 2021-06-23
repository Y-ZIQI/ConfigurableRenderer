#ifndef MAX_SEARCH_STEPS
#define MAX_SEARCH_STEPS 32
#endif

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec2 PixCoords;
out vec4 offset[3];

uniform float width;
uniform float height;

const vec4 pixel_size = vec4(1.0/width, 1.0/height, width, height);

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
    TexCoords = aTexCoords;
    PixCoords = aTexCoords * pixel_size.zw;
    offset[0] = pixel_size.xyxy * vec4(-0.25, 0.125, 1.25, 0.125) + aTexCoords.xyxy;
    offset[1] = pixel_size.xyxy * vec4(-0.125, 0.25, -0.125, -1.25) + aTexCoords.xyxy;
    offset[2] = pixel_size.xxyy * (vec4(-2.0, 2.0, 2.0, -2.0) * MAX_SEARCH_STEPS + vec4(-0.001, 0.001, 0.001, -0.001)) + vec4(offset[0].xz, offset[1].yw);
}