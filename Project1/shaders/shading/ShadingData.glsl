#ifndef __SHADING_DATA
#define __SHADING_DATA
/*struct LightSample
{
    vec3 diffuse;   // The light intensity at the surface location used for the diffuse term
    vec3 specular;  // The light intensity at the surface location used for the specular term. For light probes, the diffuse and specular components are different
    vec3 L;         // The direction from the surface to the light source
    vec3 posW;      // The world-space position of the light
    float NdotH;      // Unclamped, can be negative
    float NdotL;      // Unclamped, can be negative
    float LdotH;      // Unclamped, can be negative
    float dist;   // Distance from the light-source to the surface
};

struct ShadingData
{
    vec3  posW;                   ///< Shading hit position in world space
    vec3  V;                      ///< Direction to the eye at shading hit
    vec3  N;                      ///< Shading normal at shading hit
    vec3  T;                      ///< Shading tangent at shading hit
    vec3  B;                      ///< Shading bitangent at shading hit
    vec2  uv;                     ///< Texture mapping coordinates
    float   NdotV;                  // Unclamped, can be negative.

    // Primitive data
    vec3  faceN;                  ///< Face normal in world space, always on the front-facing side.
    bool    frontFacing;            ///< True if primitive seen from the front-facing side.
    bool    doubleSided;            ///< Material double-sided flag, if false only shade front face.

    // Pre-loaded texture data
    uint    materialID;             ///< Material ID at shading location.
    vec3  diffuse;                ///< Diffuse albedo.
    float   opacity;
    vec3  specular;               ///< Specular albedo.
    float   linearRoughness;        ///< This is the original roughness, before re-mapping.
    float   ggxAlpha;               ///< DEPRECATED: This is the re-mapped roughness value, which should be used for GGX computations. It equals `roughness^2`
    vec3  emissive;
    vec4  occlusion;
    float   IoR;                    ///< Index of refraction of the medium "behind" the surface.
    float   metallic;               ///< Metallic parameter, blends between dielectric and conducting BSDFs.
    float   specularTransmission;   ///< Specular transmission, blends between opaque dielectric BRDF and specular transmissive BSDF.
    float   eta;                    ///< Relative index of refraction (incident IoR / transmissive IoR).

    // Sampling/evaluation data
    //uint    activeLobes;            ///< BSDF lobes to include for sampling and evaluation. See LobeType in BxDFTypes.slang.

    //[mutating] void setActiveLobes(uint lobes) { activeLobes = lobes; }
};*/

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
    vec3 diffuse;                ///< Diffuse albedo.
    vec3 specular;               ///< Specular albedo.
    float linearRoughness;        ///< This is the original roughness, before re-mapping.
    float ggxAlpha;               ///< DEPRECATED: This is the re-mapped roughness value, which should be used for GGX computations. It equals `roughness^2`
    float metallic;               ///< Metallic parameter, blends between dielectric and conducting BSDFs.
    float lineardep;               ///< Linear depth to camera.
    float ao;               ///< Ambient Occlusion.
};
#endif