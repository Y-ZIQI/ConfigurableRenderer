//++`shaders/shading/defines.glsl`

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
} 

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

/* N = mix(N', N, roughness), this function returns H*/
vec3 visibleImportanceSampling(vec2 Xi, mat3 TBN, float roughness, out float pdf){
    float a = max(0.0064, roughness*roughness);

    float phi = 2.0 * M_PI * Xi.y;
    float a2 = a * a, a2_1 = a2 + 1.0;
    float cosTheta = sqrt((a2_1 - Xi.x) / ((a2 - 1.0) * Xi.x + a2_1));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    pdf = cosTheta;

    vec3 sampleVec = TBN[0] * H.x + TBN[1] * H.y + TBN[2] * H.z;
    return normalize(sampleVec);
}

vec3 visibleImportanceSampling2(vec2 Xi, vec3 N, float roughness, float range, out float pdf){
    float a = roughness*roughness;

    float phi = 2.0 * M_PI * Xi.y;
    float a2 = a * a, a3_a = (a2 + 1.0) * range;
    float cosTheta = sqrt((a3_a - Xi.x) / ((a2 - 1.0) * Xi.x + a3_a));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    pdf = cosTheta;
    
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a2 = pow(roughness, 4.0);
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (M_PI * d * d);
}