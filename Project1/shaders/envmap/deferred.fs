//++`shaders/shading/defines.glsl`

layout (location = 0) out vec4 fAlbedo;

in vec3 TexCoords;

uniform bool target_2d;
uniform samplerCube envmap;
uniform sampler2D envmap2d;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    if(target_2d){
        vec2 uv = SampleSphericalMap(normalize(TexCoords));
        fAlbedo = texture(envmap2d, uv);
    }else
        fAlbedo = texture(envmap, TexCoords);
    ATOMIC_COUNTER_I_INCREMENT(0)
}