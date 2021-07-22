#pragma once

#include "framebuffer.h"
#include "shader.h"

#ifdef min(a,b)
#undef min(a,b)
#endif
#ifdef max(a,b)
#undef max(a,b)
#endif

#define DOTVEC3(a,b) ((a[0]*b[0])+(a[1]*b[1])+(a[2]*b[2]))

#define BASECOLOR_BIT   0x1
#define SPECULAR_BIT    0x2
#define NORMAL_BIT      0x4
#define EMISSIVE_BIT    0x8

using namespace std;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Material {
    string name;
    bool texBased = true;
    int has_tex, use_tex;
    Texture* baseColorMap = nullptr;
    Texture* specularMap = nullptr;
    Texture* normalMap = nullptr;
    Texture* emissiveMap = nullptr;
    //Texture* heightMap;

    glm::vec4 baseColor;
    glm::vec3 specular;
    glm::vec3 emissive;

    void loadTexture(aiMaterial* mat, aiTextureType type, const std::string& directory = "", bool genMipmap = true, bool gammaCorrection = false, bool vfilp = true) {
        int tCount = mat->GetTextureCount(type);
        if (tCount <= 0) return;
        aiString str;
        mat->GetTexture(type, 0, &str);
        Texture * ntex = Texture::createFromFile(str.C_Str(), directory, genMipmap, gammaCorrection, vfilp);
        ntex->path = str.C_Str();
        switch (type) {
        case aiTextureType_DIFFUSE:
            baseColorMap = ntex; break;
        case aiTextureType_SPECULAR:
            specularMap = ntex; break;
        case aiTextureType_NORMALS:
            normalMap = ntex; break;
        case aiTextureType_EMISSIVE:
            emissiveMap = ntex; break;
        //case aiTextureType_AMBIENT:
        //    heightMap = ntex; break;
        }
    }
    void loadMaterialTextures(aiMaterial* mat, const std::string& directory = "", bool vfilp = true) {
        aiString str;
        mat->Get(AI_MATKEY_NAME, str);
        name = str.C_Str();
        loadTexture(mat, aiTextureType_DIFFUSE, directory, true, true, vfilp);
        loadTexture(mat, aiTextureType_SPECULAR, directory, true, false, vfilp);
        loadTexture(mat, aiTextureType_NORMALS, directory, true, false, vfilp);
        loadTexture(mat, aiTextureType_EMISSIVE, directory, true, false, vfilp);
        baseColor = glm::vec4(1.0);
        specular = glm::vec3(0.5);
        emissive = glm::vec3(0.0);
        use_tex = ~0x0;
        has_tex = 0x0;
        if (baseColorMap) has_tex |= BASECOLOR_BIT;
        if (specularMap) has_tex |= SPECULAR_BIT;
        if (normalMap) has_tex |= NORMAL_BIT;
        if (emissiveMap) has_tex |= EMISSIVE_BIT;
    }
};

class Mesh {
public:
    // mesh Data
    Material* material;
    uint voffset, ioffset;
    uint n_vertices, n_indices, n_triangles;

    struct AABB {
        glm::vec3 minB, maxB; // min and max bound for XYZ
        glm::vec3 center;
        glm::vec3 points[8];
    };

    // Properties
    uint alphaMode = 0;
    bool has_aabb = false;
    AABB aabb, aabb_transform;

    glm::mat4 model_mat;
    glm::mat3 n_model_mat;
    glm::mat4 transform;
    glm::mat3 n_transform;

    // constructor
    Mesh(uint voffset, uint ioffset, uint n_vertices, uint n_indices, Material* material) {
        this->voffset = voffset;
        this->ioffset = ioffset;
        this->n_vertices = n_vertices;
        this->n_indices = n_indices;
        this->n_triangles = n_indices / 3;
        this->material = material;
    }

    // render the mesh
    bool Draw(Shader &shader, bool pre_cut_off = false, glm::vec3 camera_pos = glm::vec3(0.0, 0.0, 0.0), glm::vec3 camera_front = glm::vec3(0.0, 0.0, 0.0))
    {
        // Pre cut off
        if (pre_cut_off && has_aabb) {
            bool is_cut_off = true;
            for (uint i = 0; i < 8; i++) {
                glm::vec3 vec = aabb_transform.points[i] - camera_pos;
                if (DOTVEC3(vec, camera_front) > 0.0f) {
                    is_cut_off = false;
                    break;
                }
            }
            if (is_cut_off)
                return false;
        }
        shader.setMat4("model", model_mat);
        shader.setMat3("normal_model", n_model_mat);
        int has_tex = material->has_tex & material->use_tex;
        shader.setInt("has_tex", has_tex);
        if(has_tex & BASECOLOR_BIT)
            shader.setTextureSource("baseColorMap", 0, material->baseColorMap->id);
        else
            shader.setVec4("const_color", material->baseColor);
        if (has_tex & SPECULAR_BIT)
            shader.setTextureSource("specularMap", 1, material->specularMap->id);
        else
            shader.setVec3("const_specular", material->specular);
        if (has_tex & NORMAL_BIT)
            shader.setTextureSource("normalMap", 2, material->normalMap->id);
        if (has_tex & EMISSIVE_BIT)
            shader.setTextureSource("emissiveMap", 3, material->emissiveMap->id);
        else
            shader.setVec3("const_emissive", material->emissive);
        // draw mesh
        glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, (void*)(ioffset * sizeof(uint)));
        return true;
    }
    void DrawMesh(Shader &shader){
        shader.setMat4("model", model_mat);
        glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, (void*)(ioffset * sizeof(uint)));
    }
    void setModelMat(glm::mat4 new_mat, glm::mat3 new_n_mat) {
        model_mat = transform * new_mat;
        n_model_mat = n_transform * new_n_mat;
        aabb_transform.minB = glm::vec3(model_mat * glm::vec4(aabb.minB, 1.0));
        aabb_transform.maxB = glm::vec3(model_mat * glm::vec4(aabb.maxB, 1.0));
        aabb_transform.center = glm::vec3(model_mat * glm::vec4(aabb.center, 1.0));
        for (uint i = 0; i < 8; i++)
            aabb_transform.points[i] = glm::vec3(model_mat * glm::vec4(aabb.points[i], 1.0));
    }
    void setAABB(glm::vec3 minB, glm::vec3 maxB) {
        aabb = { minB, maxB, glm::vec3(0.5f, 0.5f, 0.5f) * (minB + maxB) };
        aabb.points[0] = aabb.minB;
        aabb.points[1] = glm::vec3(aabb.minB[0], aabb.minB[1], aabb.maxB[2]);
        aabb.points[2] = glm::vec3(aabb.minB[0], aabb.maxB[1], aabb.minB[2]);
        aabb.points[3] = glm::vec3(aabb.minB[0], aabb.maxB[1], aabb.maxB[2]);
        aabb.points[4] = glm::vec3(aabb.maxB[0], aabb.minB[1], aabb.minB[2]);
        aabb.points[5] = glm::vec3(aabb.maxB[0], aabb.minB[1], aabb.maxB[2]);
        aabb.points[6] = glm::vec3(aabb.maxB[0], aabb.maxB[1], aabb.minB[2]);
        aabb.points[7] = aabb.maxB;
        has_aabb = true;
    }
};