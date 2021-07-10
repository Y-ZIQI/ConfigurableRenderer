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
    Texture* baseColorMap = nullptr;
    Texture* specularMap = nullptr;
    Texture* normalMap = nullptr;
    //Texture* heightMap;
    //Texture* emissiveMap;

    glm::vec4 baseColor;
    glm::vec4 specular;
    //glm::vec4 emissive;

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
        baseColor = glm::vec4(1.0);
        specular = glm::vec4(0.5);
    }
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<uint>        indices;
    vector<Texture*>      textures;
    Material* material;
    uint VAO;

    struct AABB {
        glm::vec3 minB, maxB; // min and max bound for XYZ
        glm::vec3 center;
        glm::vec3 points[8];
    };

    // Properties
    uint triangles;
    uint alphaMode;
    bool has_aabb = false;
    AABB aabb, aabb_transform;

    glm::mat4 model_mat;
    glm::mat3 n_model_mat;
    glm::mat4 transform;
    glm::mat3 n_transform;

    // constructor
    Mesh(vector<Vertex> vertices, vector<uint> indices, Material* material)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->material = material;

        setupMesh();
        setupProperities();
        setupAABB();
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
        shader.setBool("has_normalmap", material->normalMap != nullptr);
        shader.setBool("tex_based", material->texBased);
        if (material->texBased) {
            if (material->baseColorMap)
                shader.setTextureSource("baseColorMap", 0, material->baseColorMap->id);
            if (material->specularMap)
                shader.setTextureSource("specularMap", 1, material->specularMap->id);
        }
        else {
            shader.setVec4("const_color", material->baseColor);
            shader.setVec4("const_specular", material->specular);
        }
        if (material->normalMap)
            shader.setTextureSource("normalMap", 2, material->normalMap->id);
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        return true;
    }
    void DrawMesh(Shader &shader){
        shader.setMat4("model", model_mat);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
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

private:
    // render data 
    uint VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
    }
    void setupProperities() {
        triangles = indices.size() / 3;
        alphaMode = 0;
        /*for (uint i = 0; i < textures.size(); i++)
        {
            if (textures[i]->type == "texture_diffuse" && textures[i]->format == GL_RGBA) {
                alphaMode = 1;
            }
        }*/
    }
    void setupAABB() {
        // Center defined by (min+max)/2
        aabb = { vertices[0].Position ,vertices[0].Position ,glm::vec3(0.5f, 0.5f, 0.5f) };
        for (uint i = 1; i < vertices.size(); i++) {
            aabb.minB = glm::min(aabb.minB, vertices[i].Position);
            aabb.maxB = glm::max(aabb.maxB, vertices[i].Position);
        }
        aabb.center *= aabb.minB + aabb.maxB;
        // Center defined by centroid
        /*aabb = { vertices[0].Position ,vertices[0].Position ,glm::vec3(0.0f, 0.0f, 0.0f) };
        for (uint i = 1; i < vertices.size(); i++) {
            aabb.minB = glm::min(aabb.minB, vertices[i].Position);
            aabb.maxB = glm::max(aabb.maxB, vertices[i].Position);
            aabb.center += vertices[i].Position;
        }
        aabb.center /= (float)vertices.size();*/
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