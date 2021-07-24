#pragma once

#include "framebuffer.h"
#include "shader.h"

class ShadowMap {
public:
    bool initialized = false;
    FrameBuffer* smBuffer;
    uint width, height;

    ShadowMap(uint Width, uint Height) {
        initialized = true;
        width = Width; height = Height;
        smBuffer = new FrameBuffer;
        smBuffer->addDepthStencil(width, height);
        Texture* ntex = Texture::create(width, height, GL_R32F, GL_RED, GL_FLOAT);
        //ntex->setTexParami(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        ntex->setTexParami(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //ntex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
        //ntex->setTexParami(GL_TEXTURE_MAX_LEVEL, 4);
        smBuffer->attachColorTarget(ntex, 0);
        //ntex->setTexParami(GL_TEXTURE_REDUCTION_MODE_ARB, GL_MAX);
    };
};

enum class LType : int {
    Directional = 1,
    Point = 2,
};

class Light {
public:
    std::string name;
    LType type;

    float ambient;
    glm::vec3 intensity;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 target;

    bool shadow_enabled = false;
    float light_size = 3.0f;
    std::vector<ShadowMap*> smList;
    ShadowMap* shadowMap;
    Shader* smShader;
    glm::mat4 viewProj, viewMat, projMat;

    nanogui::ref<nanogui::Window> lightWindow;

    void prepareShadow() {
        if (!shadow_enabled) return;
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, shadowMap->width, shadowMap->height);
        shadowMap->smBuffer->clear();
        shadowMap->smBuffer->prepare();
        smShader->use();
        smShader->setMat4("viewProj", viewProj);
    }
    void addGui(nanogui::FormHelper* gui, nanogui::ref<nanogui::Window> sceneWindow) {
        gui->addButton(name, [this]() {
            lightWindow->setFocused(!lightWindow->visible());
            lightWindow->setVisible(!lightWindow->visible()); 
        })->setIcon(type == LType::Directional ? ENTYPO_ICON_LIGHT_UP : ENTYPO_ICON_LIGHT_BULB);
        lightWindow = gui->addWindow(Eigen::Vector2i(500, 0), name);
        lightWindow->setWidth(250);
        lightWindow->setVisible(false);
        gui->addGroup("ambient");
        auto tbox = gui->addVariable("ambient", ambient);
        tbox->setSpinnable(true);
        tbox->setValueIncrement(0.01);
        gui->addGroup("Intensity");
        gui->addVariable("Intensity.r", intensity[0])->setSpinnable(true);
        gui->addVariable("Intensity.g", intensity[1])->setSpinnable(true);
        gui->addVariable("Intensity.b", intensity[2])->setSpinnable(true);
        gui->addGroup("Position");
        gui->addVariable("Position.x", position[0])->setSpinnable(true);
        gui->addVariable("Position.y", position[1])->setSpinnable(true);
        gui->addVariable("Position.z", position[2])->setSpinnable(true);
        gui->addGroup("Direction");
        gui->addVariable("Direction.x", direction[0])->setSpinnable(true);
        gui->addVariable("Direction.y", direction[1])->setSpinnable(true);
        gui->addVariable("Direction.z", direction[2])->setSpinnable(true);
        gui->addGroup("Close");
        gui->addButton("Close", [this]() { lightWindow->setVisible(false); })->setIcon(ENTYPO_ICON_CROSS);
        gui->setWindow(sceneWindow);
    }
};

class DirectionalLight : public Light {
public:
    // For shadow map
    float rangeX, rangeY, rangeZ;

    DirectionalLight(
        const std::string& Name, 
        glm::vec3 Intensity, 
        glm::vec3 Direction, 
        glm::vec3 Position = glm::vec3(0.0, 0.0, 0.0), 
        float Ambient = 0.01f
    ) {
        type = LType::Directional;
        name = Name;
        ambient = Ambient;
        intensity = Intensity;
        direction = Direction;
        position = Position;
        target = Position + Direction;
    }
    bool update(uint gen_shadow = 1, glm::vec3 cam_pos = glm::vec3(0.0, 0.0, 0.0), glm::vec3 cam_front = glm::vec3(0.0, 0.0, 0.0)) {
        static glm::mat4 previous_mat = glm::mat4(0.0);
        const float threshold = 10.0f;
        bool genShadow = false;
        if (shadow_enabled) {
            if (gen_shadow == 1) {
                glm::vec3 cam_target = cam_pos + cam_front * rangeX * 0.5f;
                float dist = glm::length(target - cam_target);
                if (dist > threshold) {
                    focus_on(cam_target);
                    genShadow = true;
                }
            }
            else if (gen_shadow == 2) {
                glm::vec3 cam_target = cam_pos + cam_front * rangeX * 0.5f;
                focus_on(cam_target);
                genShadow = true;
            }
            viewMat = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
            projMat = glm::ortho(-rangeX * 0.5f, rangeX * 0.5f, -rangeY * 0.5f, rangeY * 0.5f, 0.0f, rangeZ);
            viewProj = projMat * viewMat;
            if (previous_mat != viewProj) {
                previous_mat = viewProj;
                genShadow = true;
            }
            if (genShadow) prepareShadow();
        }
        return genShadow;
    }
    void focus_on(glm::vec3 target) {
        this->target = target;
        if (shadow_enabled) position = target - direction * rangeZ * 0.5f;
        else position = target - direction * 1.0f;
    }
    void enableShadow(float rangeX, float rangeY, float rangeZ, std::vector<uint> width_list) {
        if (!shadow_enabled) {
            shadow_enabled = true;
            smList.resize(width_list.size());
            for(uint i = 0;i < width_list.size();i++)
                smList[i] = new ShadowMap(width_list[i], width_list[i]);
            shadowMap = smList[0];
            smShader = sManager.getShader(SID_SHADOWMAP);
        }
        this->rangeX = rangeX;
        this->rangeY = rangeY;
        this->rangeZ = rangeZ;
        update();
    }
    void setShadowMap(uint idx) {
        if (shadow_enabled)
            shadowMap = smList[idx];
    }
    void setUniforms(Shader& shader, uint index, uint& sm_index) {
        char tmp[64];
        sprintf(tmp, "dirLights[%d].intensity", index);
        shader.setVec3(tmp, intensity);
        sprintf(tmp, "dirLights[%d].direction", index);
        shader.setVec3(tmp, direction);
        sprintf(tmp, "dirLights[%d].ambient", index);
        shader.setFloat(tmp, ambient);
        sprintf(tmp, "dirLights[%d].has_shadow", index);
        shader.setBool(tmp, shadow_enabled);
        if (shadow_enabled) {
            sprintf(tmp, "dirLights[%d].viewProj", index);
            shader.setMat4(tmp, viewProj);
            sprintf(tmp, "dirLights[%d].shadowMap", index);
            shader.setTextureSource(tmp, sm_index++, shadowMap->smBuffer->colorAttachs[0].texture->id);
            sprintf(tmp, "dirLights[%d].resolution", index);
            shader.setFloat(tmp, (float)shadowMap->width);
            sprintf(tmp, "dirLights[%d].light_size", index);
            shader.setFloat(tmp, light_size / rangeX);
        }
    }
};

class PointLight : public Light {
public:
    // For light fading
    float range, constant, linear, quadratic;
    float opening_angle, penumbra_angle;
    // For shadow map
    float nearZ, farZ, Zoom, Aspect, Frustum;

    PointLight(
        const std::string& Name, 
        glm::vec3 Intensity, 
        glm::vec3 Position, 
        glm::vec3 Direction = glm::vec3(0.0, 0.0, 0.0), 
        float Ambient = 0.01f, 
        float Range = 10.0f, 
        float Opening_angle = 180.0f, 
        float Penumbra_angle = 0.0f
    ) {
        type = LType::Point;
        name = Name;
        ambient = Ambient;
        intensity = Intensity;
        position = Position;
        direction = Direction;
        target = Position + Direction;
        range = Range;
        opening_angle = Opening_angle * M_PI / 180.0f;
        penumbra_angle = Penumbra_angle * M_PI / 180.0f;
        //constant = 1.0f;
        constant = 0.01f;
        linear = 4.0f / Range;
        quadratic = 70.0f / (Range * Range);
    }
    bool update(uint gen_shadow = 1) {
        static glm::mat4 previous_mat = glm::mat4(0.0);
        bool genShadow = false;
        if (shadow_enabled) {
            if (gen_shadow == 2) genShadow = true;
            viewMat = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
            projMat = glm::perspective(glm::radians(Zoom), Aspect, nearZ, farZ);
            viewProj = projMat * viewMat;
            if (previous_mat != viewProj) genShadow = true;
            if (genShadow) prepareShadow();
        }
        return genShadow;
    }
    void enableShadow(float Zoom, float nearZ, float farZ, std::vector<uint> width_list) {
        if (!shadow_enabled) {
            shadow_enabled = true;
            smList.resize(width_list.size());
            for (uint i = 0; i < width_list.size(); i++)
                smList[i] = new ShadowMap(width_list[i], width_list[i]);
            shadowMap = smList[0];
            smShader = sManager.getShader(SID_SHADOWMAP);
        }
        this->nearZ = nearZ;
        this->farZ = farZ;
        this->Zoom = Zoom;
        this->Aspect = 1.0f;
        this->Frustum = 2.0f * nearZ * tanf(glm::radians(Zoom) / 2.0f);
        update();
    }
    void setShadowMap(uint idx) {
        if (shadow_enabled)
            shadowMap = smList[idx];
    }
    void setUniforms(Shader& shader, uint index, uint& sm_index) {
        char tmp[64];
        sprintf(tmp, "ptLights[%d].intensity", index);
        shader.setVec3(tmp, intensity);
        sprintf(tmp, "ptLights[%d].position", index);
        shader.setVec3(tmp, position);
        sprintf(tmp, "ptLights[%d].direction", index);
        shader.setVec3(tmp, direction);
        sprintf(tmp, "ptLights[%d].ambient", index);
        shader.setFloat(tmp, ambient);
        sprintf(tmp, "ptLights[%d].range", index);
        shader.setFloat(tmp, range);
        sprintf(tmp, "ptLights[%d].opening_angle", index);
        shader.setFloat(tmp, opening_angle);
        sprintf(tmp, "ptLights[%d].cos_opening_angle", index);
        shader.setFloat(tmp, cosf(opening_angle));
        sprintf(tmp, "ptLights[%d].penumbra_angle", index);
        shader.setFloat(tmp, penumbra_angle);
        sprintf(tmp, "ptLights[%d].constant", index);
        shader.setFloat(tmp, constant);
        sprintf(tmp, "ptLights[%d].linear", index);
        shader.setFloat(tmp, linear);
        sprintf(tmp, "ptLights[%d].quadratic", index);
        shader.setFloat(tmp, quadratic);
        sprintf(tmp, "ptLights[%d].has_shadow", index);
        shader.setBool(tmp, shadow_enabled);
        if (shadow_enabled) {
            sprintf(tmp, "ptLights[%d].viewProj", index);
            shader.setMat4(tmp, viewProj);
            sprintf(tmp, "ptLights[%d].shadowMap", index);
            shader.setTextureSource(tmp, sm_index++, shadowMap->smBuffer->colorAttachs[0].texture->id);
            sprintf(tmp, "ptLights[%d].resolution", index);
            shader.setFloat(tmp, (float)shadowMap->width);
            sprintf(tmp, "dirLights[%d].light_size", index);
            shader.setFloat(tmp, light_size / Frustum);
        }
    }
};