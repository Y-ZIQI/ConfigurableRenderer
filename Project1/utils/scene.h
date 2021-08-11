#pragma once
#include "light.h"
#include "model.h"
#include "camera.h"
#include "texture.h"
#include "ibl.h"

using namespace std;

class Scene {
public:
    string name;
    Camera* camera;
    vector<Camera*> cameras;
    vector<Model*> models;
    vector<DirectionalLight*> dirLights;
    vector<PointLight*> ptLights;
    vector<RadioactiveLight*> radioLights;
    vector<IBL*> light_probes;
    float ssao_range, ssao_bias, ssao_threshold, ssr_threshold;

    vector<Texture*> envMaps;
    uint envIdx;
    bool envMap_enabled = false;

    jsonxx::json jscene;
    bool new_loaded;

    Scene() {};
    ~Scene() {
        clearScene();
    }
    void addCamera(Camera* newCamera) { cameras.push_back(newCamera); }
    void setCamera(Camera* newCamera) { camera = newCamera; };
    void addLight(PointLight* nLight) { ptLights.push_back(nLight); };
    void addLight(DirectionalLight* nLight) { dirLights.push_back(nLight); };
    void addLight(RadioactiveLight* nLight) { radioLights.push_back(nLight); };
    void addModel(Model* nModel) { models.push_back(nModel); };
    void loadModel(string const& path, const string name, bool vflip = true) { addModel(new Model(path, name, vflip)); };
    void setModelMat(uint idx, glm::mat4 &nMat) {
        if (idx < models.size())
            models[idx]->setModelMat(nMat);
    }
    void addEnvMap(Texture* envMap) {
        if (!envMap_enabled) envMap_enabled = true;
        envMaps.push_back(envMap); 
    };
    void setEnvMap(uint idx) { envIdx = idx; };
    void addLightProbe(IBL* nLight) { light_probes.push_back(nLight); };
    void Draw(Shader& shader, uint pre_cut = 0) {
        glEnable(GL_DEPTH_TEST);
        for (uint i = 0; i < models.size(); i++) {
            if(pre_cut == 0)
                models[i]->Draw(shader, true, camera->Position, camera->Front);
            else
                models[i]->Draw(shader, false);
        }
    }
    void DrawMesh(Shader& shader) {
        glEnable(GL_DEPTH_TEST);
        for (uint i = 0; i < models.size(); i++) {
            models[i]->DrawMesh(shader);
        }
    }
    void DrawEnvMap(Shader& shader) {
        shader.use();
        setCameraUniforms(shader, true);
        if (envMaps[envIdx]->target == GL_TEXTURE_CUBE_MAP) {
            shader.setBool("target_2d", false);
            shader.setTextureSource("envmap", 0, envMaps[envIdx]->id, GL_TEXTURE_CUBE_MAP);
            shader.setTextureSource("envmap2d", 1, 0, GL_TEXTURE_2D);
        }
        else {
            shader.setBool("target_2d", true);
            shader.setTextureSource("envmap", 0, 0, GL_TEXTURE_CUBE_MAP);
            shader.setTextureSource("envmap2d", 1, envMaps[envIdx]->id, GL_TEXTURE_2D);
        }
        glDepthFunc(GL_LEQUAL);
        renderCube();
        glDepthFunc(GL_LESS);
    }
    void update(uint gen_shadow = 1, bool filtering = false) {
        camera->update();
        const bool cull_front = false;
        if(cull_front) glCullFace(GL_FRONT);
        for (int i = 0; i < dirLights.size(); i++)
            if (dirLights[i]->update(gen_shadow, camera->Position, camera->Front)) {
                dirLights[i]->prepareShadow();
                DrawMesh(*dirLights[i]->smShader);
                if (filtering)dirLights[i]->filterShadow();
            }
        for (int i = 0; i < ptLights.size(); i++)
            if (ptLights[i]->update(gen_shadow)) {
                ptLights[i]->prepareShadow();
                DrawMesh(*ptLights[i]->smShader);
                if (filtering)ptLights[i]->filterShadow();
            }
        for (int i = 0; i < radioLights.size(); i++)
            if (radioLights[i]->update(gen_shadow)) {
                radioLights[i]->prepareShadow();
                DrawMesh(*radioLights[i]->smShader);
                if (filtering)radioLights[i]->filterShadow();
            }
        if (cull_front) glCullFace(GL_BACK);
    }
    void setLightUniforms(Shader& shader, bool eval_ibl = true) {
        shader.setInt("dirLtCount", dirLights.size());
        shader.setInt("ptLtCount", ptLights.size());
        shader.setInt("radioLtCount", radioLights.size());
        uint t_index = GBUFFER_TARGETS;
        for (uint i = 0; i < dirLights.size(); i++)
            dirLights[i]->setUniforms(shader, i, t_index);
        for (uint i = 0; i < ptLights.size(); i++)
            ptLights[i]->setUniforms(shader, i, t_index);
        for (uint i = 0; i < radioLights.size(); i++)
            radioLights[i]->setUniforms(shader, i, t_index);
        /* All samplerCube must be initialized, or some errors occured */
        for (uint i = radioLights.size(); i < MAX_RADIOACTIVE_LIGHT; i++) {
            shader.setInt(getStrFormat("radioLights[%d].shadowMap", i), DEFAULT_CUBE_MAP_SOURCE);
        }
        /***************************************************************/

        if (eval_ibl) {
            uint lp_count = 0;
            shader.setTextureSource("brdfLUT", t_index++, tManager.getTex(TID_BRDFLUT)->id, tManager.getTex(TID_BRDFLUT)->target);
            for (uint i = 0; i < light_probes.size(); i++) {
                if (light_probes[i]->processed) {
                    lp_count++;
                    light_probes[i]->setUniforms(shader, i, t_index);
                }
            }
            shader.setInt("iblCount", lp_count);
            /* All samplerCube must be initialized, or some errors occured */
            for (uint i = lp_count; i < MAX_IBL_LIGHT; i++) {
                shader.setInt(getStrFormat("ibls[%d].prefilterMap", i), DEFAULT_CUBE_MAP_SOURCE);
            }
            /***************************************************************/
        }
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
    void loadScene(string const& path, const string name = "scene") {
        try{
            std::ifstream ifs(path);
            ifs >> jscene;
        }
        catch (std::ifstream::failure& e){
            std::cout << "ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ" << std::endl;
            return;
        }
        this->name = name;
        clearScene();

        try {
            ssao_range = jscene["ssao_range"].as_float();
            ssao_bias = jscene["ssao_bias"].as_float();
            ssao_threshold = jscene["ssao_threshold"].as_float();
            ssr_threshold = jscene["ssr_threshold"].as_float();

            std::string directory = path.substr(0, path.find_last_of('/')) + '/';
            uint mid = 0;
            auto models = jscene["models"].as_array();
            for (uint i = 0; i < models.size(); i++) {
                std::string mpath = models[i]["file"].as_string();
                std::string mname = models[i]["name"].as_string();
                bool texture_filp_vertically = models[i]["texture_filp_vertically"].as_bool();
                std::string texture_sampler = models[i]["texture_sampler"].as_string();
                mpath = directory + mpath;
                auto instances = models[i]["instances"].as_array();
                for (uint j = 0; j < instances.size(); j++) {
                    loadModel(mpath, mname, texture_filp_vertically);
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
                else if (files.is_string()) {
                    std::string skybox_path = directory + files.as_string();
                    skybox = Texture::createHDRMapFromFile(skybox_path, true);
                }
                addEnvMap(skybox);
            }
            setEnvMap(0);

            auto light_probes = jscene["light_probes"].as_array();
            for (uint i = 0; i < light_probes.size(); i++) {
                auto name = light_probes[i]["name"].as_string();
                auto file = light_probes[i]["file"].as_string();
                if (file != "") file = directory + file;
                auto intensity = light_probes[i]["intensity"].as_array();
                auto pos = light_probes[i]["pos"].as_array();
                auto range = light_probes[i]["range"].as_float();
                auto depth_range = light_probes[i]["depth_range"].as_array();
                addLightProbe(new IBL(
                    name, file,
                    glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
                    glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                    range, depth_range[0].as_float(), depth_range[1].as_float()
                ));
            }

            auto lights = jscene["lights"].as_array();
            for (uint i = 0; i < lights.size(); i++) {
                std::string name = lights[i]["name"].as_string();
                std::string type = lights[i]["type"].as_string();
                auto intensity = lights[i]["intensity"].as_array();
                auto pos = lights[i]["pos"].as_array();
                float ambient = lights[i]["ambient"].as_float();
                float light_size = lights[i]["light_size"].as_float();
                auto shadow = lights[i]["shadow"];
                bool shadow_enable = shadow["enable"].as_bool();
                if (type == "dir_light") {
                    auto direction = lights[i]["direction"].as_array();
                    DirectionalLight* nLight = new DirectionalLight(
                        name,
                        glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
                        glm::vec3(direction[0].as_float(), direction[1].as_float(), direction[2].as_float()),
                        glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                        ambient, light_size
                    );
                    if (shadow_enable) {
                        float rangeX = shadow["rangeX"].as_float();
                        float rangeY = shadow["rangeY"].as_float();
                        float rangeZ = shadow["rangeZ"].as_float();
                        nLight->enableShadow(rangeX, rangeY, rangeZ, { 512, 1024, 2048 });
                    }
                    addLight(nLight);
                }
                else if (type == "point_light") {
                    auto direction = lights[i]["direction"].as_array();
                    float range = lights[i]["range"].as_float();
                    float opening_angle = lights[i]["opening_angle"].as_float();
                    float penumbra_angle = lights[i]["penumbra_angle"].as_float();

                    PointLight* nLight = new PointLight(
                        name,
                        glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
                        glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                        glm::vec3(direction[0].as_float(), direction[1].as_float(), direction[2].as_float()),
                        ambient, range, opening_angle, penumbra_angle, light_size
                    );
                    if (shadow_enable) {
                        float fovy = shadow["fovy"].as_float();
                        float nearz = shadow["nearz"].as_float();
                        float farz = shadow["farz"].as_float();
                        nLight->enableShadow(fovy, nearz, farz, { 512, 1024, 2048 });
                    }
                    addLight(nLight);
                }
                else if (type == "radio_light") {
                    float range = lights[i]["range"].as_float();
                    RadioactiveLight* nLight = new RadioactiveLight(
                        name,
                        glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
                        glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
                        ambient, range, light_size
                    );
                    if (shadow_enable) {
                        float nearz = shadow["nearz"].as_float();
                        float farz = shadow["farz"].as_float();
                        nLight->enableShadow(nearz, farz, { 256, 512, 1024 });
                    }
                    addLight(nLight);
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
            new_loaded = true;
        }
        catch (jsonxx::json_exception& e) {
            std::cout << e.what() << std::endl;
            return;
        }
    }
    void clearScene() {
        for (uint i = 0; i < cameras.size(); i++) 
            if (cameras[i])delete cameras[i];
        cameras.clear();
        for (uint i = 0; i < models.size(); i++) 
            if (models[i])delete models[i];
        models.clear();
        for (uint i = 0; i < envMaps.size(); i++) 
            if (envMaps[i])delete envMaps[i];
        envMaps.clear();
        for (uint i = 0; i < dirLights.size(); i++) 
            if (dirLights[i])delete dirLights[i];
        dirLights.clear();
        for (uint i = 0; i < ptLights.size(); i++) 
            if (ptLights[i])delete ptLights[i];
        ptLights.clear();
        for (uint i = 0; i < radioLights.size(); i++) 
            if (radioLights[i])delete radioLights[i];
        radioLights.clear();
        for (uint i = 0; i < light_probes.size(); i++) 
            if (light_probes[i])delete light_probes[i];
        light_probes.clear();
    };
    bool getLoadedStatue(bool reset = true) {
        if (reset && new_loaded)
            return !(new_loaded = false);
        return new_loaded;
    }
    void renderGui() {
        if (ImGui::TreeNode("Models")) {
            for (uint i = 0; i < models.size(); i++)
                models[i]->renderGui();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Lights")) {
            for (uint i = 0; i < dirLights.size(); i++)
                dirLights[i]->renderGui();
            for (uint i = 0; i < ptLights.size(); i++)
                ptLights[i]->renderGui();
            for (uint i = 0; i < radioLights.size(); i++)
                radioLights[i]->renderGui();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Light probes")) {
            for (uint i = 0; i < light_probes.size(); i++)
                light_probes[i]->renderGui();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Camera")) {
            camera->renderGui();
            ImGui::TreePop();
        }
    }
};