//++`shaders/shading/defines.glsl`

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube envmap;

#define SAMPLE_DELTA 0.025

void main()
{       
    vec3 normal = normalize(TexCoords);

    vec3 irradiance = vec3(0.0);

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up         = cross(normal, right);

    float sampleDelta = 0.025;
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
            irradiance += texture(envmap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = M_PI * irradiance * (1.0 / float(nrSamples));

    FragColor = vec4(irradiance, 1.0);
}