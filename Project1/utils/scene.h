#pragma once
#include "light.h"
#include "model.h"
#include "camera.h"
#include "texture.h"
#include "ibl.h"

using namespace std;

class Scene {
public:
    Camera* camera;
    vector<Camera*> cameras;
    vector<Model*> models;
    vector<PointLight*> ptLights;
    vector<DirectionalLight*> dirLights;
    vector<glm::mat4> modelMats;
    vector<glm::mat3> normalModelMats;

    vector<Texture*> envMaps;
    uint envIdx;
    bool envMap_enabled = false;

    vector<IBL*> light_probes;

    jsonxx::json jscene;

    TimeRecord record[2]; // GenShadow, Draw

    Scene() {};
    void addCamera(Camera* newCamera) { cameras.push_back(newCamera); }
    void setCamera(Camera* newCamera) { camera = newCamera; };
    void addLight(PointLight* nLight) { ptLights.push_back(nLight); };
    void addLight(DirectionalLight* nLight) { dirLights.push_back(nLight); };
    void addModel(Model* nModel) {
        models.push_back(nModel);
        modelMats.push_back(glm::mat4(1.0));
        normalModelMats.push_back(glm::mat4(1.0));
    };
    void loadModel(string const& path, bool gamma = false) {
        Model* m = new Model(path, gamma);
        addModel(m);
    }
    void setModelMat(uint idx, glm::mat4 &nMat) {
        if (idx >= modelMats.size() || idx >= normalModelMats.size())
            return;
        modelMats[idx] = nMat;
        normalModelMats[idx] = glm::transpose(glm::inverse(glm::mat3(nMat)));
        models[idx]->setModelMat(nMat);
    }
    void addEnvMap(Texture* envMap) {
        if (!envMap_enabled) envMap_enabled = true;
        envMaps.push_back(envMap); 
    };
    void setEnvMap(uint idx) { envIdx = idx; };
    void addLightProbe(IBL* nLight) { light_probes.push_back(nLight); };
    void Draw(Shader& shader, uint blend_mode = 0, bool draw_envmap = true) {
        if (blend_mode == 0) {
            for (uint i = 0; i < models.size(); i++) {
                setModelUniforms(shader, i);
                //models[i]->Draw(shader, false);
                models[i]->Draw(shader, true, modelMats[i], camera->Position, camera->Front);
            }
            if (draw_envmap && envMap_enabled && envIdx >= 0)
                DrawEnvMap(*sManager.getShader(
                    envMaps[envIdx]->target == GL_TEXTURE_CUBE_MAP ? SID_DEFERRED_ENVMAP : SID_DEFERRED_ENVMAP2D
                ));
        }else{
            for (uint i = 0; i < models.size(); i++) {
                setModelUniforms(shader, i);
                models[i]->DrawOpaque(shader);
            }
            if (draw_envmap && envMap_enabled && envIdx >= 0)
                DrawEnvMap(*sManager.getShader(
                    envMaps[envIdx]->target == GL_TEXTURE_CUBE_MAP ? SID_DEFERRED_ENVMAP : SID_DEFERRED_ENVMAP2D
                ));
            glm::mat4 viewProj = camera->GetViewProjMatrix();
            shader.use();
            for (uint i = 0; i < models.size(); i++) {
                models[i]->OrderTransparent(viewProj * modelMats[i]);
                models[i]->DrawTransparent(shader);
            }
        }
    }
    void DrawMesh(Shader& shader) {
        for (uint i = 0; i < models.size(); i++) {
            setModelUniforms(shader, i, false);
            models[i]->DrawMesh(shader);
        }
    }
    void DrawEnvMap(Shader& shader) {
        checkEnvmapVAO();
        shader.use();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        setCameraUniforms(shader, true);
        shader.setTextureSource("envmap", 0, envMaps[envIdx]->id, envMaps[envIdx]->target);
        glBindVertexArray(_envmap_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        frame_record.triangles += 12;
        frame_record.draw_calls += 1;
    }
    void update(bool gen_shadow = true) {
        camera->update();
        for (int i = 0; i < dirLights.size(); i++)
            dirLights[i]->update(gen_shadow, camera->Position, camera->Front);
        for (int i = 0; i < ptLights.size(); i++)
            ptLights[i]->update();
        if(gen_shadow)
            genShadow();
    }
    void genShadow(bool cull_front = false) {
        if(cull_front)
            glCullFace(GL_FRONT);
        ShadowMap* sm_ptr;
        for (int i = 0; i < dirLights.size(); i++) {
            if (dirLights[i]->shadow_enabled) {
                sm_ptr = dirLights[i]->shadowMap;
                glEnable(GL_DEPTH_TEST);
                glViewport(0, 0, sm_ptr->width, sm_ptr->height);
                sm_ptr->smBuffer->clear();
                sm_ptr->smBuffer->prepare();
                dirLights[i]->smShader->use();
                dirLights[i]->smShader->setMat4("viewProj", dirLights[i]->viewProj);
                DrawMesh(*dirLights[i]->smShader);
                sm_ptr->smBuffer->colorAttachs[0].texture->genMipmap();
            }
        }
        for (int i = 0; i < ptLights.size(); i++) {
            if (ptLights[i]->shadow_enabled) {
                sm_ptr = ptLights[i]->shadowMap;
                glEnable(GL_DEPTH_TEST);
                glViewport(0, 0, sm_ptr->width, sm_ptr->height);
                sm_ptr->smBuffer->clear();
                sm_ptr->smBuffer->prepare();
                ptLights[i]->smShader->use();
                ptLights[i]->smShader->setMat4("viewProj", ptLights[i]->viewProj);
                DrawMesh(*ptLights[i]->smShader);
                sm_ptr->smBuffer->colorAttachs[0].texture->genMipmap();
            }
        }
        if (cull_front)
            glCullFace(GL_BACK);
    }
    void setModelUniforms(Shader& shader, uint model_index, bool normal_model = true) {
        if (normal_model)
            shader.setMat3("normal_model", normalModelMats[model_index]);
        shader.setMat4("model", modelMats[model_index]);
    }
    void setLightUniforms(Shader& shader) {
        shader.setInt("dirLtCount", dirLights.size());
        shader.setInt("ptLtCount", ptLights.size());
        uint sm_index = 5;
        for (uint i = 0; i < dirLights.size(); i++)
            dirLights[i]->setUniforms(shader, i, sm_index);
        for (uint i = 0; i < ptLights.size(); i++)
            ptLights[i]->setUniforms(shader, i, sm_index);

        shader.setInt("iblCount", light_probes.size());
        for (uint i = 0; i < light_probes.size(); i++)
            light_probes[i]->setUniforms(shader, i, sm_index);
    }
    void setCameraUniforms(Shader& shader, bool remove_trans = false) {
        float inv_n = 1.0f / camera->nearZ, inv_f = 1.0f / camera->farZ;
        glm::mat4 projectionMat = camera->GetProjMatrix();
        glm::mat4 viewMat = camera->GetViewMatrix();
        if (remove_trans)
            viewMat = glm::mat4(glm::mat3(viewMat));
        shader.setVec3("camera_pos", camera->Position);
        shader.setVec4("camera_params", glm::vec4(camera->Zoom, camera->Aspect, inv_n + inv_f, inv_n - inv_f));
        shader.setMat4("camera_vp", projectionMat * viewMat);
    }
    void loadScene(string const& path) {
        try{
            std::ifstream ifs(path);
            ifs >> jscene;
        }
        catch (std::ifstream::failure& e){
            std::cout << "ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ" << std::endl;
            return;
        }
        clearScene();
        std::string directory = path.substr(0, path.find_last_of('/')) + '/';
        uint mid = 0;
        auto models = jscene["models"].as_array();
        for (uint i = 0; i < models.size(); i++) {
            std::string mpath = models[i]["file"].as_string();
            mpath = directory + mpath;
            auto instances = models[i]["instances"].as_array();
            for (uint j = 0; j < instances.size(); j++) {
                loadModel(mpath);
                auto translation = instances[j]["translation"].as_array();
                auto scaling = instances[j]["scaling"].as_array();
                auto rotation = instances[j]["rotation"].as_array();
                glm::mat4 modelmat = glm::mat4(1.0);
                modelmat = glm::translate(modelmat, glm::vec3(translation[0].as_float(), translation[1].as_float(), translation[2].as_float()));
                modelmat = glm::scale(modelmat, glm::vec3(scaling[0].as_float(), scaling[1].as_float(), scaling[2].as_float()));
                //TODO: Fix Rotate
                modelmat = glm::rotate(modelmat, glm::radians((float)rotation[0].as_float()), glm::vec3(1.0f, 0.0f, 0.0f));
                modelmat = glm::rotate(modelmat, glm::radians((float)rotation[1].as_float()), glm::vec3(0.0f, 1.0f, 0.0f));
                modelmat = glm::rotate(modelmat, glm::radians((float)rotation[2].as_float()), glm::vec3(0.0f, 0.0f, 1.0f));
                setModelMat(mid++, modelmat);
            }
        }

        auto env_maps = jscene["environment_maps"].as_array();
        for (uint i = 0; i < env_maps.size(); i++) {
            auto files = env_maps[i]["files"];
            Texture* skybox;
            if (files.is_array()) {
                std::vector<std::string> skybox_path
                {
                    directory + files[0].as_string(),
                    directory + files[1].as_string(),
                    directory + files[2].as_string(),
                    directory + files[3].as_string(),
                    directory + files[4].as_string(),
                    directory + files[5].as_string()
                };
                skybox = Texture::createCubeMapFromFile(skybox_path, false);
            }
            else if(files.is_string()) {
                std::string skybox_path = directory + files.as_string();
                skybox = Texture::createHDRMapFromFile(skybox_path, true);
            }
            addEnvMap(skybox);
        }
        setEnvMap(0);

        auto light_probes = jscene["light_probes"].as_array();
        for (uint i = 0; i < light_probes.size(); i++) {
            auto file = directory + light_probes[i]["file"].as_string();
            auto intensity = light_probes[i]["intensity"].as_array();
            auto pos = light_probes[i]["pos"].as_array();
            addLightProbe(new IBL(
                file, glm::vec3(intensity[0], intensity[1], intensity[2]), glm::vec3(pos[0], pos[1], pos[2])
            ));
        }

        auto lights = jscene["lights"].as_array();
        for (uint i = 0; i < lights.size(); i++) {
            std::string name = lights[i]["name"].as_string();
            std::string type = lights[i]["type"].as_string();
            auto intensity = lights[i]["intensity"].as_array();
            auto direction = lights[i]["direction"].as_array();
            auto pos = lights[i]["pos"].as_array();
            float ambient = lights[i]["ambient"].as_float();
            if (type == "dir_light") {
                addLight(new DirectionalLight(
                    name,
                    glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
                    glm::vec3(direction[0].as_float(), direction[1].as_float(), direction[2].as_float()),
                    glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                    ambient
                ));
            }
            else if (type == "point_light") {
                float range = lights[i]["range"].as_float();
                addLight(new PointLight(
                    name,
                    glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
                    glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                    glm::vec3(direction[0].as_float(), direction[1].as_float(), direction[2].as_float()),
                    ambient, range
                ));
            }
        }

        auto cameras = jscene["cameras"].as_array();
        for (uint i = 0; i < cameras.size(); i++) {
            //std::string name = cameras[i]["name"].as_string();
            auto pos = cameras[i]["pos"].as_array();
            auto target = cameras[i]["target"].as_array();
            auto up = cameras[i]["up"].as_array();
            auto depth_range = cameras[i]["depth_range"].as_array();
            float focal_length = cameras[i]["focal_length"].as_float();
            float aspect_ratio = cameras[i]["aspect_ratio"].as_float();
            float camera_speed = cameras[i]["camera_speed"].as_float();
            Camera* nCamera = new Camera(
                glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                glm::vec3(target[0].as_float(), target[1].as_float(), target[2].as_float()),
                glm::vec3(up[0].as_float(), up[1].as_float(), up[2].as_float()),
                focal_length, aspect_ratio, depth_range[0].as_float(), depth_range[1].as_float(), camera_speed
            );
            addCamera(nCamera);
        }
        setCamera(this->cameras[0]);
    }
    void clearScene() {
        //TODO: clear loaded data
    };
};