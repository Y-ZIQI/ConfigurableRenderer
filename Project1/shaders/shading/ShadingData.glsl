#ifndef __SHADING_DATA
#define __SHADING_DATA
struct LightSample{
    vec3 color;
    vec3 L;         // The direction from the surface to the light source
    vec3 posW;      // The world-space position of the light
    float NdotH;
    float NdotL;
    float LdotH;
    float dist;
};
struct ShadingData{
    vec3 posW;                   ///< Shading hit position in world space
    vec3 V;                      ///< Direction to the eye at shading hit
    vec3 N;                      ///< Shading normal at shading hit
    float NdotV;                  // Unclamped, can be negative.
    vec3 baseColor;                ///< Diffuse albedo.
    vec3 diffuse;                ///< Diffuse albedo.
    vec3 specular;               ///< Specular albedo.
    float linearRoughness;        ///< This is the original roughness, before re-mapping.
    float ggxAlpha;               ///< DEPRECATED: This is the re-mapped roughness value, which should be used for GGX computations. It equals `roughness^2`
    float metallic;               ///< Metallic parameter, blends between dielectric and conducting BSDFs.
    float lineardep;               ///< Linear depth to camera.
    float ao;               ///< Ambient Occlusion.
};
#endif
