//++`shaders/shading/defines.glsl`

layout (location = 0) out vec4 outColor;

in vec3 TexCoords;

uniform samplerCube envmap;

#define SAMPLE_DELTA 0.025
#define INF 100.0

void main()
{       
    vec3 normal = normalize(TexCoords);

    vec3 irradiance = vec3(0.0);

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up         = cross(normal, right);

    int nrSamples = 0; 
    for(float phi = 0.0; phi < 2.0 * M_PI; phi += SAMPLE_DELTA)
    {
        for(float theta = 0.0; theta < 0.5 * M_PI; theta += SAMPLE_DELTA)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

            ATOMIC_COUNT_INCREMENT
            vec3 color = clamp(texture(envmap, sampleVec).rgb, -INF, INF);
            irradiance += color * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = M_PI * irradiance * (1.0 / float(nrSamples));

    outColor = vec4(irradiance, 1.0);
    ATOMIC_COUNT_CALCULATE
}