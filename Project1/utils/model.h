#pragma once

#include "mesh.h"
#include "shader.h"

using namespace std;

class Model 
{
public:
    // model data 
    vector<Texture*> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh*>    meshes;
    vector<uint>    oIndex;
    vector<uint>    tIndex;

    struct T_Ptr {
        uint index;
        float dist;
    };
    vector<T_Ptr>    tOrder;

    struct Node {
        string name;
        glm::mat4 transform;
        uint parent;
        vector<uint> children;
        vector<uint> meshes;
    };
    vector<Node> graph;

    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(uint i = 0; i < meshes.size(); i++)
            meshes[i]->Draw(shader);
    }
    void DrawOpaque(Shader& shader) {
        glDisable(GL_BLEND);
        for (uint i = 0; i < oIndex.size(); i++)
            meshes[oIndex[i]]->Draw(shader);
    }
    void DrawTransparent(Shader& shader) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        for (uint i = 0; i < tOrder.size(); i++)
            meshes[tIndex[tOrder[i].index]]->Draw(shader);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }
    void DrawMesh(Shader &shader){
        for(uint i = 0; i < meshes.size(); i++)
            meshes[i]->DrawMesh(shader);
    }
    void OrderTransparent(glm::mat4 transform) {
        for (uint i = 0; i < tOrder.size(); i++) {
            tOrder[i] = { i, (transform * glm::vec4(meshes[tIndex[i]]->aabb.center, 1.0f)).z };
        }
        std::sort(tOrder.begin(), tOrder.end(), [](T_Ptr a, T_Ptr b) { return a.dist >= b.dist; });
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        meshes.resize(scene->mNumMeshes);
        for (uint i = 0; i < scene->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[i];
            meshes[i] = processMesh(mesh, scene);
        }
        // process ASSIMP's root node recursively
        graph.resize(1);
        processNode(scene->mRootNode, scene, 0, 0);
        tOrder.resize(tIndex.size());
    }

    glm::mat4 aiCast(const aiMatrix4x4& aiMat)
    {
        glm::mat4 glmMat;
        glmMat[0][0] = aiMat.a1; glmMat[0][1] = aiMat.a2; glmMat[0][2] = aiMat.a3; glmMat[0][3] = aiMat.a4;
        glmMat[1][0] = aiMat.b1; glmMat[1][1] = aiMat.b2; glmMat[1][2] = aiMat.b3; glmMat[1][3] = aiMat.b4;
        glmMat[2][0] = aiMat.c1; glmMat[2][1] = aiMat.c2; glmMat[2][2] = aiMat.c3; glmMat[2][3] = aiMat.c4;
        glmMat[3][0] = aiMat.d1; glmMat[3][1] = aiMat.d2; glmMat[3][2] = aiMat.d3; glmMat[3][3] = aiMat.d4;
        return transpose(glmMat);
    }

    void processNode(aiNode* node, const aiScene* scene, uint nid, uint parentid) {
        graph[nid].name = node->mName.C_Str();
        graph[nid].transform = aiCast(node->mTransformation);
        graph[nid].parent = parentid;
        graph[nid].meshes.resize(node->mNumMeshes);
        for (uint i = 0; i < node->mNumMeshes; i++)
        {
            uint meshID = node->mMeshes[i];
            graph[nid].meshes[i] = meshID;
        }
        graph[nid].children.resize(node->mNumChildren);
        for (uint i = 0; i < node->mNumChildren; i++)
        {
            Node n;
            graph.push_back(n);
            processNode(node->mChildren[i], scene, graph.size() - 1, nid);
        }
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(uint i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            Mesh* nm = processMesh(mesh, scene);
            meshes.push_back(nm);
            (nm->alphaMode == 0 ? oIndex : tIndex).push_back(meshes.size() - 1);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(uint i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh* processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<uint> indices;
        vector<Texture*> textures;

        // walk through each of the mesh's vertices
        for(uint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(uint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(uint j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture*> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", true);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture*> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", false);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        // aiTextureType_HEIGHT are normal maps type if load .obj
        //std::vector<Texture*> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        std::vector<Texture*> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", false);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture*> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", false);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return new Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture*> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName, bool gammaCorrection = false)
    {
        vector<Texture*> textures;
        for(uint i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(uint j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j]->path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture* texture = Texture::createFromFile(str.C_Str(), this->directory, true, gammaCorrection, true);
                //Texture* texture = Texture::createFromFile(str.C_Str(), this->directory, true, false, false);
                texture->type = typeName;
                texture->path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};