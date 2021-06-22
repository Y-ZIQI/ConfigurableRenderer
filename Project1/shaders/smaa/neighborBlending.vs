layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 offset;

uniform float width;
uniform float height;

uniform float resolution;

const vec4 pixel_size = vec4(1.0/width, 1.0/height, width, height);

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
    TexCoords = aTexCoords * resolution;
    offset = fma(pixel_size.xyxy, vec4( 1.0, 0.0, 0.0, -1.0), aTexCoords.xyxy * resolution);
}