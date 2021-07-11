#pragma once

#include "mesh.h"
#include "shader.h"

using namespace std;

class Model 
{
public:
    string name;
    // model data 
    vector<Material*> materials;
    vector<Mesh*>    meshes;

    struct Node {
        string name;
        glm::mat4 transform;
        uint parent;
        vector<uint> children;
        vector<uint> meshes;
    };
    vector<Node> graph;

    vector<uint>    oIndex;
    vector<uint>    tIndex;
    struct T_Ptr {
        uint index;
        float dist;
    };
    vector<T_Ptr>    tOrder;

    string directory;
    bool gammaCorrection;

    uint triangles;
    glm::mat4 model_mat;
    glm::mat3 n_model_mat;

    nanogui::ref<nanogui::Window> modelWindow;
    vector<nanogui::ref<nanogui::Window>> matWindows;

    Model(string const &path, const string name, bool gamma = false) : gammaCorrection(gamma)
    {
        this->name = name;
        loadModel(path);
    }

    void Draw(Shader &shader, bool pre_cut_off = false, glm::vec3 camera_pos = glm::vec3(0.0, 0.0, 0.0), glm::vec3 camera_front = glm::vec3(0.0, 0.0, 0.0))
    {
        for (uint i = 0; i < meshes.size(); i++) {
            if (meshes[i]->Draw(shader, pre_cut_off, camera_pos, camera_front)) {
                frame_record.triangles += meshes[i]->triangles;
                frame_record.draw_calls += 1;
            }
        }
    }
    void DrawMesh(Shader &shader){
        for (uint i = 0; i < meshes.size(); i++) {
            meshes[i]->DrawMesh(shader);
            frame_record.triangles += meshes[i]->triangles;
            frame_record.draw_calls += 1;
        }
    }
    void DrawOpaque(Shader& shader) {
        glDisable(GL_BLEND);
        for (uint i = 0; i < oIndex.size(); i++)
            if (meshes[oIndex[i]]->Draw(shader)) {
                frame_record.triangles += meshes[oIndex[i]]->triangles;
                frame_record.draw_calls += 1;
            }
    }
    void DrawTransparent(Shader& shader) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        for (uint i = 0; i < tOrder.size(); i++)
            if (meshes[tIndex[tOrder[i].index]]->Draw(shader)) {
                frame_record.triangles += meshes[tIndex[tOrder[i].index]]->triangles;
                frame_record.draw_calls += 1;
            }
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }
    void OrderTransparent(glm::mat4 viewproj) {
        for (uint i = 0; i < tOrder.size(); i++) {
            tOrder[i] = { i, (viewproj * glm::vec4(meshes[tIndex[i]]->aabb.center, 1.0f)).z };
        }
        std::sort(tOrder.begin(), tOrder.end(), [](T_Ptr a, T_Ptr b) { return a.dist >= b.dist; });
    }
    void setModelMat(glm::mat4 new_mat) {
        model_mat = new_mat;
        n_model_mat = glm::transpose(glm::inverse(glm::mat3(new_mat)));
        for (uint i = 0; i < meshes.size(); i++)
            meshes[i]->setModelMat(model_mat, n_model_mat);
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

        // Add Materials
        materials.resize(scene->mNumMaterials);
        for (uint i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            materials[i] = new Material;
            materials[i]->loadMaterialTextures(mat, directory, true);
        }
        // Add Meshes
        meshes.resize(scene->mNumMeshes);
        for (uint i = 0; i < scene->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[i];
            meshes[i] = processMesh(mesh, scene);
        }
        // process ASSIMP's root node recursively
        graph.resize(1);
        processNode(scene->mRootNode, scene, 0, 0, glm::mat4(1.0f));
    }

    glm::mat4 aiCast(const aiMatrix4x4& aiMat)
    {
        glm::mat4 glmMat;
        glmMat[0][0] = aiMat.a1; glmMat[0][1] = aiMat.a2; glmMat[0][2] = aiMat.a3; glmMat[0][3] = aiMat.a4;
        glmMat[1][0] = aiMat.b1; glmMat[1][1] = aiMat.b2; glmMat[1][2] = aiMat.b3; glmMat[1][3] = aiMat.b4;
        glmMat[2][0] = aiMat.c1; glmMat[2][1] = aiMat.c2; glmMat[2][2] = aiMat.c3; glmMat[2][3] = aiMat.c4;
        glmMat[3][0] = aiMat.d1; glmMat[3][1] = aiMat.d2; glmMat[3][2] = aiMat.d3; glmMat[3][3] = aiMat.d4;
        return glmMat;
    }

    void processNode(aiNode* node, const aiScene* scene, uint nid, uint parentid, glm::mat4 transform) {
        graph[nid].name = node->mName.C_Str();
        graph[nid].transform = aiCast(node->mTransformation);
        graph[nid].parent = parentid;
        graph[nid].meshes.resize(node->mNumMeshes);
        glm::mat4 nTrans = graph[nid].transform * transform;
        for (uint i = 0; i < node->mNumMeshes; i++)
        {
            uint meshID = node->mMeshes[i];
            graph[nid].meshes[i] = meshID;
            meshes[meshID]->transform = glm::transpose(nTrans);
            meshes[meshID]->n_transform = glm::inverse(glm::mat3(nTrans));
        }
        graph[nid].children.resize(node->mNumChildren);
        for (uint i = 0; i < node->mNumChildren; i++)
        {
            Node n;
            graph.push_back(n);
            processNode(node->mChildren[i], scene, graph.size() - 1, nid, nTrans);
        }
    }

    Mesh* processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<uint> indices;
        for(uint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
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
            if(mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
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
        for(uint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for(uint j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials 
        Material* material = materials[mesh->mMaterialIndex];
        return new Mesh(vertices, indices, material);
    }

public:
    void addGui(nanogui::FormHelper* gui, nanogui::ref<nanogui::Window> sceneWindow) {
        gui->addButton(name, [this]() {
            modelWindow->setFocused(!modelWindow->visible());
            modelWindow->setVisible(!modelWindow->visible()); 
        });
        modelWindow = gui->addWindow(Eigen::Vector2i(500, 0), name);
        modelWindow->setWidth(250);
        modelWindow->setVisible(false);
        gui->addGroup("Transform");
        //gui->addVariable("Direction.x", direction[0])->setSpinnable(true);
        //gui->addVariable("Direction.y", direction[1])->setSpinnable(true);
        //gui->addVariable("Direction.z", direction[2])->setSpinnable(true);
        gui->addGroup("Materials");
        nanogui::PopupButton* matBtn = new nanogui::PopupButton(modelWindow, "Materials");
        nanogui::VScrollPanel* vscroll = new nanogui::VScrollPanel(matBtn->popup());
        nanogui::Widget* wd = new nanogui::Widget(vscroll);
        wd->setLayout(new nanogui::GroupLayout());
        nanogui::Button* btn;
        nanogui::CheckBox* cb;
        matWindows.resize(materials.size());
        for (uint i = 0; i < materials.size(); i++) {
            matWindows[i] = gui->addWindow(Eigen::Vector2i(750, 0), materials[i]->name);
            gui->addWidget("", new nanogui::CheckBox(matWindows[i], "BaseColor tex", [this, i](bool state) {
                materials[i]->use_tex = state ? (materials[i]->use_tex | BASECOLOR_BIT) : (materials[i]->use_tex & ~BASECOLOR_BIT);
            }));
            gui->addWidget("", new nanogui::CheckBox(matWindows[i], "Specular tex", [this, i](bool state) {
                materials[i]->use_tex = state ? (materials[i]->use_tex | SPECULAR_BIT) : (materials[i]->use_tex & ~SPECULAR_BIT);
            }));
            gui->addWidget("", new nanogui::CheckBox(matWindows[i], "Normal tex", [this, i](bool state) {
                materials[i]->use_tex = state ? (materials[i]->use_tex | NORMAL_BIT) : (materials[i]->use_tex & ~NORMAL_BIT);
            }));
            gui->addWidget("", new nanogui::CheckBox(matWindows[i], "Emissive tex", [this, i](bool state) {
                materials[i]->use_tex = state ? (materials[i]->use_tex | EMISSIVE_BIT) : (materials[i]->use_tex & ~EMISSIVE_BIT);
            }));
            gui->addVariable("BaseColor.r", materials[i]->baseColor[0])->setSpinnable(true);
            gui->addVariable("BaseColor.g", materials[i]->baseColor[1])->setSpinnable(true);
            gui->addVariable("BaseColor.b", materials[i]->baseColor[2])->setSpinnable(true);
            gui->addVariable("Roughness", materials[i]->specular[1])->setSpinnable(true);
            gui->addVariable("Metallic", materials[i]->specular[2])->setSpinnable(true);
            gui->addButton("Close", [this, i]() { matWindows[i]->setVisible(false); })->setIcon(ENTYPO_ICON_CROSS);
            matWindows[i]->setVisible(false);
            btn = new nanogui::Button(wd, materials[i]->name);
            btn->setCallback([this, i]() {matWindows[i]->setVisible(!matWindows[i]->visible()); });
            gui->setWindow(modelWindow);
        }
        matBtn->popup()->setFixedHeight(500);
        gui->addWidget("", matBtn);
        gui->addGroup("Close");
        gui->addButton("Close", [this]() { modelWindow->setVisible(false); })->setIcon(ENTYPO_ICON_CROSS);
        gui->setWindow(sceneWindow);
    }
};