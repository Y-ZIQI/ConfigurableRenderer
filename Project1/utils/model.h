#pragma once

#include "mesh.h"
#include "shader.h"

using namespace std;

class Model 
{
public:
    string name;
    string directory;
    // model data 
    uint VAO = UINT_MAX, VBO = UINT_MAX, EBO = UINT_MAX;
    uint n_vertices, n_indices;
    vector<Vertex> vertices;
    vector<uint> indices;
    vector<Material*> materials;
    vector<Mesh*> meshes;

    glm::mat4 model_mat;
    glm::mat3 n_model_mat;

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

    nanogui::ref<nanogui::Window> modelWindow;
    vector<nanogui::ref<nanogui::Window>> matWindows;

    Model(string const &path, const string name, bool vflip = true)
    {
        this->name = name;
        loadModel(path, vflip);
        setupModel();
    }
    ~Model() {
        if (glIsVertexArray(VAO))glDeleteVertexArrays(1, &VAO);
        if (glIsBuffer(VBO))glDeleteBuffers(1, &VBO);
        if (glIsBuffer(EBO))glDeleteBuffers(1, &EBO);
        for (uint i = 0; i < materials.size(); i++) if (materials[i])delete materials[i];
        for (uint i = 0; i < meshes.size(); i++) if (meshes[i])delete meshes[i];
    };

    void Draw(Shader &shader, bool pre_cut_off = false, glm::vec3 camera_pos = glm::vec3(0.0, 0.0, 0.0), glm::vec3 camera_front = glm::vec3(0.0, 0.0, 0.0))
    {
        glBindVertexArray(VAO);
        for (uint i = 0; i < meshes.size(); i++) {
            if (meshes[i]->Draw(shader, pre_cut_off, camera_pos, camera_front)) {
                frame_record.triangles += meshes[i]->n_triangles;
                frame_record.draw_calls += 1;
            }
        }
        glBindVertexArray(0);
    }
    void DrawMesh(Shader &shader){
        glBindVertexArray(VAO);
        for (uint i = 0; i < meshes.size(); i++) {
            meshes[i]->DrawMesh(shader);
            frame_record.triangles += meshes[i]->n_triangles;
            frame_record.draw_calls += 1;
        }
        glBindVertexArray(0);
    }
    void DrawOpaque(Shader& shader) {
        glDisable(GL_BLEND);
        glBindVertexArray(VAO);
        for (uint i = 0; i < oIndex.size(); i++)
            if (meshes[oIndex[i]]->Draw(shader)) {
                frame_record.triangles += meshes[oIndex[i]]->n_triangles;
                frame_record.draw_calls += 1;
            }
        glBindVertexArray(0);
    }
    void DrawTransparent(Shader& shader) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(VAO);
        for (uint i = 0; i < tOrder.size(); i++)
            if (meshes[tIndex[tOrder[i].index]]->Draw(shader)) {
                frame_record.triangles += meshes[tIndex[tOrder[i].index]]->n_triangles;
                frame_record.draw_calls += 1;
            }
        glBindVertexArray(0);
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
    void loadModel(string const &path, bool vflip = true)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));

        // Add Materials
        materials.resize(scene->mNumMaterials);
        for (uint i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            materials[i] = new Material;
            materials[i]->loadMaterialTextures(mat, directory, vflip);
        }
        // Add Meshes
        meshes.resize(scene->mNumMeshes);
        n_vertices = n_indices = 0;
        vertices.resize(0);
        indices.resize(0);
        for (uint i = 0; i < scene->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[i];
            uint vertices_i = mesh->mNumVertices, indices_i = 0;
            glm::vec3 minB{ mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z };
            glm::vec3 maxB{ mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z };
            for (uint i = 0; i < vertices_i; i++){
                Vertex vertex;
                vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                if (mesh->HasNormals())
                    vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                if (mesh->mTextureCoords[0]){
                    vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                    vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                    vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                }else
                    vertex.TexCoords = glm::vec2(0.0f, 0.0f);
                minB = glm::min(minB, vertex.Position);
                maxB = glm::max(maxB, vertex.Position);
                vertices.push_back(vertex);
            }
            for (uint i = 0; i < mesh->mNumFaces; i++){
                aiFace face = mesh->mFaces[i];
                for (uint j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j] + n_vertices);
                indices_i += face.mNumIndices;
            }
            meshes[i] = new Mesh(n_vertices, n_indices, vertices_i, indices_i, materials[mesh->mMaterialIndex]);
            meshes[i]->setAABB(minB, maxB);
            n_vertices += vertices_i;
            n_indices += indices_i;
        }
        // process ASSIMP's root node recursively
        graph.resize(1);
        processNode(scene->mRootNode, scene, 0, 0, glm::mat4(1.0f));
    }

    void setupModel() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, n_vertices * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indices * sizeof(uint), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
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
            gui->addVariable("Emissive.r", materials[i]->emissive[0])->setSpinnable(true);
            gui->addVariable("Emissive.g", materials[i]->emissive[1])->setSpinnable(true);
            gui->addVariable("Emissive.b", materials[i]->emissive[2])->setSpinnable(true);
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