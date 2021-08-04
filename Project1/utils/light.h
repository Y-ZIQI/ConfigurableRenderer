#pragma once

#include "framebuffer.h"
#include "shader.h"

class BaseShadowMap {
public:
    bool initialized = false;
    FrameBuffer* smBuffer;
    Texture* smTex[2];
    uint texId;
    uint width, height;
    BaseShadowMap() {};
};

class ShadowMap : public BaseShadowMap {
public:
    ShadowMap(uint Width, uint Height) {
        initialized = true;
        width = Width; height = Height;
        smBuffer = new FrameBuffer;
        smBuffer->addDepthStencil(width, height);
        smTex[0] = Texture::create(width, height, GL_R32F, GL_RED, GL_FLOAT, GL_NEAREST); // For pcss/hard
        smTex[1] = Texture::create(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR); // For evsm
        texId = 1;
        smBuffer->attachColorTarget(smTex[texId], 0);
        smBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR), 1); // Buffer for filter
    };
    void setTex(uint id) {
        if (texId != id) {
            texId = id;
            smBuffer->attachColorTarget(smTex[id], 0);
        }
    }
};
class OmniShadowMap : public BaseShadowMap {
public:
    glm::vec2 pixel_size;

    OmniShadowMap(uint Width, uint Height) {
        initialized = true;
        width = Width; height = Height;
        pixel_size = glm::vec2(2.0 / (float)Width, 2.0 / (float)Height);
        smBuffer = new FrameBuffer;
        smBuffer->setParami(GL_FRAMEBUFFER_DEFAULT_LAYERS, 6);
        smBuffer->attachCubeDepthTarget(Texture::createCubeMap(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST));
        smTex[0] = Texture::createCubeMap(width, height, GL_R32F, GL_RED, GL_FLOAT, GL_NEAREST); // For pcss/hard
        smTex[1] = Texture::createCubeMap(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR); // For evsm
        texId = 0;
        smBuffer->attachCubeTarget(smTex[texId], 0);
        smBuffer->attachCubeTarget(Texture::createCubeMap(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_LINEAR), 1); // Buffer for filter
    };
    void setTex(uint id) {
        if (texId != id) {
            texId = id;
            smBuffer->attachCubeTarget(smTex[id], 0);
        }
    }
};

enum class LType : int {
    Directional = 1,
    Point = 2,
    Radioactive = 3
};

class BaseLight {
public:
    std::string name;
    LType type;
    // For shading
    float ambient;
    glm::vec3 intensity;
    glm::vec3 position;
    // For shadow
    bool shadow_enabled = false;
    float light_size;
    // For GUI
    nanogui::ref<nanogui::Window> lightWindow;

    BaseLight() {};
    void addGui(nanogui::FormHelper* gui) {
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
    }
};
class Light : public BaseLight {
public:
    // For shading
    glm::vec3 direction;
    glm::vec3 target;
    // For shadow
    std::vector<ShadowMap*> smList;
    ShadowMap* shadowMap;
    Shader* smShader, *filterShader;
    glm::mat4 viewProj, viewMat, projMat, previousMat;

    void setShadowMap(uint idx) {
        if (shadow_enabled)
            shadowMap = smList[idx];
    }
    void setShadowMapTex(uint idx) {
        if (shadow_enabled)
            for (uint i = 0; i < smList.size(); i++)
                smList[i]->setTex(idx);
    }
    void prepareShadow() {
        if (!shadow_enabled) return;
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, shadowMap->width, shadowMap->height);
        smShader->use();
        smShader->setMat4("viewProj", viewProj);
        shadowMap->smBuffer->prepare(0);
        shadowMap->smBuffer->clear();
    }
    void filterShadow() {
        filterShader->use();
        filterShader->setInt("ksize", (shadowMap->width / 512) + 1);
        filterShader->setBool("horizontal", true);
        filterShader->setTextureSource("colorTex", 0, shadowMap->smBuffer->colorAttachs[0].texture->id);
        shadowMap->smBuffer->prepare(1);
        renderScreen();
        filterShader->setBool("horizontal", false);
        filterShader->setTextureSource("horizontalTex", 1, shadowMap->smBuffer->colorAttachs[1].texture->id);
        shadowMap->smBuffer->prepare(0);
        renderScreen();
    }
    void addGui(nanogui::FormHelper* gui, nanogui::ref<nanogui::Window> sceneWindow) {
        BaseLight::addGui(gui);
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
        float Ambient = 0.01f,
        float Light_size = 1.0f
    ) {
        type = LType::Directional;
        name = Name;
        ambient = Ambient;
        intensity = Intensity;
        direction = Direction;
        position = Position;
        target = Position + Direction;
        light_size = Light_size;
    }
    bool update(uint gen_shadow = 1, glm::vec3 cam_pos = glm::vec3(0.0, 0.0, 0.0), glm::vec3 cam_front = glm::vec3(0.0, 0.0, 0.0)) {
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
            viewMat = glm::lookAt(position, position + direction, abs(direction[1]) > 0.999f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f));
            projMat = glm::ortho(-rangeX * 0.5f, rangeX * 0.5f, -rangeY * 0.5f, rangeY * 0.5f, 0.0f, rangeZ);
            viewProj = projMat * viewMat;
            if (previousMat != viewProj) {
                previousMat = viewProj;
                genShadow = true;
            }
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
            filterShader = sManager.getShader(SID_SHADOWMAP_FILTER);
            previousMat = glm::mat4(0.0);
        }
        this->rangeX = rangeX;
        this->rangeY = rangeY;
        this->rangeZ = rangeZ;
        viewMat = glm::lookAt(position, position + direction, abs(direction[1]) > 0.999f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f));
        projMat = glm::ortho(-rangeX * 0.5f, rangeX * 0.5f, -rangeY * 0.5f, rangeY * 0.5f, 0.0f, rangeZ);
        viewProj = projMat * viewMat;
    }
    void setUniforms(Shader& shader, uint index, uint& sm_index) {
        shader.setVec3(getStrFormat("dirLights[%d].intensity", index), intensity);
        shader.setVec3(getStrFormat("dirLights[%d].direction", index), direction);
        shader.setFloat(getStrFormat("dirLights[%d].ambient", index), ambient);
        shader.setBool(getStrFormat("dirLights[%d].has_shadow", index), shadow_enabled);
        if (shadow_enabled) {
            shader.setMat4(getStrFormat("dirLights[%d].viewProj", index), viewProj);
            shader.setTextureSource(getStrFormat("dirLights[%d].shadowMap", index), sm_index++, shadowMap->smBuffer->colorAttachs[0].texture->id);
            shader.setFloat(getStrFormat("dirLights[%d].resolution", index), (float)shadowMap->width);
            shader.setFloat(getStrFormat("dirLights[%d].light_size", index), light_size / rangeX);
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
        float Penumbra_angle = 0.0f,
        float Light_size = 0.1f
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
        light_size = Light_size;
    }
    bool update(uint gen_shadow = 1) {
        bool genShadow = false;
        if (shadow_enabled) {
            if (gen_shadow == 2) genShadow = true;
            viewMat = glm::lookAt(position, position + direction, abs(direction[1]) > 0.999f? glm::vec3(1.0f, 0.0f, 0.0f):glm::vec3(0.0f, 1.0f, 0.0f));
            projMat = glm::perspective(glm::radians(Zoom), Aspect, nearZ, farZ);
            viewProj = projMat * viewMat;
            if (previousMat != viewProj) {
                previousMat = viewProj;
                genShadow = true;
            }
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
            filterShader = sManager.getShader(SID_SHADOWMAP_FILTER);
            previousMat = glm::mat4(0.0);
        }
        this->nearZ = nearZ;
        this->farZ = farZ;
        this->Zoom = Zoom;
        this->Aspect = 1.0f;
        this->Frustum = 2.0f * nearZ * tanf(glm::radians(Zoom) / 2.0f);
        viewMat = glm::lookAt(position, position + direction, abs(direction[1]) > 0.999f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f));
        projMat = glm::perspective(glm::radians(Zoom), Aspect, nearZ, farZ);
        viewProj = projMat * viewMat;
    }
    void setUniforms(Shader& shader, uint index, uint& sm_index) {
        shader.setVec3(getStrFormat("ptLights[%d].intensity", index), intensity);
        shader.setVec3(getStrFormat("ptLights[%d].position", index), position);
        shader.setVec3(getStrFormat("ptLights[%d].direction", index), direction);
        shader.setFloat(getStrFormat("ptLights[%d].ambient", index), ambient);
        shader.setFloat(getStrFormat("ptLights[%d].range", index), range);
        shader.setFloat(getStrFormat("ptLights[%d].opening_angle", index), opening_angle);
        shader.setFloat(getStrFormat("ptLights[%d].cos_opening_angle", index), cosf(opening_angle));
        shader.setFloat(getStrFormat("ptLights[%d].penumbra_angle", index), penumbra_angle);
        shader.setFloat(getStrFormat("ptLights[%d].constant", index), constant);
        shader.setFloat(getStrFormat("ptLights[%d].linear", index), linear);
        shader.setFloat(getStrFormat("ptLights[%d].quadratic", index), quadratic);
        shader.setBool(getStrFormat("ptLights[%d].has_shadow", index), shadow_enabled);
        if (shadow_enabled) {
            shader.setMat4(getStrFormat("ptLights[%d].viewProj", index), viewProj);
            shader.setFloat(getStrFormat("ptLights[%d].inv_n", index), 1.0f / nearZ);
            shader.setFloat(getStrFormat("ptLights[%d].inv_f", index), 1.0f / farZ);
            shader.setTextureSource(getStrFormat("ptLights[%d].shadowMap", index), sm_index++, shadowMap->smBuffer->colorAttachs[0].texture->id);
            shader.setFloat(getStrFormat("ptLights[%d].resolution", index), (float)shadowMap->width);
            shader.setFloat(getStrFormat("ptLights[%d].light_size", index), light_size);
        }
    }
};

class RadioactiveLight : public BaseLight {
public:
    // For light fading
    float range, constant, linear, quadratic;

    std::vector<OmniShadowMap*> smList;
    OmniShadowMap* shadowMap;
    Shader* smShader, *filterShader;
    // For shadow map
    float nearZ, farZ;
    glm::mat4 projMat;
    std::vector<glm::mat4> transforms;

    RadioactiveLight(
        const std::string& Name,
        glm::vec3 Intensity,
        glm::vec3 Position,
        float Ambient = 0.01f,
        float Range = 10.0f,
        float Light_size = 0.1f
    ) {
        type = LType::Radioactive;
        name = Name;
        ambient = Ambient;
        intensity = Intensity;
        position = Position;
        range = Range;
        constant = 0.01f;
        linear = 4.0f / Range;
        quadratic = 70.0f / (Range * Range);
        light_size = Light_size;
    }
    void setShadowMap(uint idx) {
        if (shadow_enabled);
            shadowMap = smList[idx];
    }
    void setShadowMapTex(uint idx) {
        if (shadow_enabled)
            for (uint i = 0; i < smList.size(); i++)
                smList[i]->setTex(idx);
    }
    void prepareShadow() {
        if (!shadow_enabled) return;
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, shadowMap->width, shadowMap->height);
        smShader->use();
        smShader->setMat4("transforms[0]", transforms[0]);
        smShader->setMat4("transforms[1]", transforms[1]);
        smShader->setMat4("transforms[2]", transforms[2]);
        smShader->setMat4("transforms[3]", transforms[3]);
        smShader->setMat4("transforms[4]", transforms[4]);
        smShader->setMat4("transforms[5]", transforms[5]);
        shadowMap->smBuffer->prepare(0);
        shadowMap->smBuffer->clear();
    }
    void filterShadow() {
        filterShader->use();
        filterShader->setMat4("transforms[0]", _capture_projection * _capture_views[0]);
        filterShader->setMat4("transforms[1]", _capture_projection * _capture_views[1]);
        filterShader->setMat4("transforms[2]", _capture_projection * _capture_views[2]);
        filterShader->setMat4("transforms[3]", _capture_projection * _capture_views[3]);
        filterShader->setMat4("transforms[4]", _capture_projection * _capture_views[4]);
        filterShader->setMat4("transforms[5]", _capture_projection * _capture_views[5]);
        filterShader->setInt("ksize", (shadowMap->width / 256) + 1);
        filterShader->setVec2("pixel_size", shadowMap->pixel_size);
        filterShader->setBool("horizontal", true);
        filterShader->setTextureSource("colorTex", 0, shadowMap->smBuffer->colorAttachs[0].texture->id, shadowMap->smBuffer->colorAttachs[0].texture->target);
        filterShader->setTextureSource("horizontalTex", 1, shadowMap->smBuffer->colorAttachs[1].texture->id, shadowMap->smBuffer->colorAttachs[1].texture->target);
        shadowMap->smBuffer->prepare(1);
        renderCube(false);
        filterShader->setBool("horizontal", false);
        shadowMap->smBuffer->prepare(0);
        renderCube(false);
    }
    bool update(uint gen_shadow = 1) {
        bool genShadow = false;
        if (shadow_enabled) {
            if (gen_shadow == 2) genShadow = true;
        }
        return genShadow;
    }
    void enableShadow(float nearZ, float farZ, std::vector<uint> width_list) {
        if (!shadow_enabled) {
            shadow_enabled = true;
            smList.resize(width_list.size());
            for (uint i = 0; i < width_list.size(); i++)
                smList[i] = new OmniShadowMap(width_list[i], width_list[i]);
            shadowMap = smList[0];
            smShader = sManager.getShader(SID_OMNISHADOWMAP);
            filterShader = sManager.getShader(SID_OMNISHADOWMAP_FILTER);
        }
        this->nearZ = nearZ;
        this->farZ = farZ;
        transforms.resize(6);
        setTransforms(glm::perspective(glm::radians(90.0f), 1.0f, nearZ, farZ));
    }
    void setTransforms(glm::mat4 proj) {
        projMat = proj;
        transforms[0] = proj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms[1] = proj * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms[2] = proj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        transforms[3] = proj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        transforms[4] = proj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms[5] = proj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    }
    void setUniforms(Shader& shader, uint index, uint& sm_index) {
        shader.setVec3(getStrFormat("radioLights[%d].intensity", index), intensity);
        shader.setVec3(getStrFormat("radioLights[%d].position", index), position);
        shader.setFloat(getStrFormat("radioLights[%d].ambient", index), ambient);
        shader.setFloat(getStrFormat("radioLights[%d].range", index), range);
        shader.setFloat(getStrFormat("radioLights[%d].constant", index), constant);
        shader.setFloat(getStrFormat("radioLights[%d].linear", index), linear);
        shader.setFloat(getStrFormat("radioLights[%d].quadratic", index), quadratic);
        shader.setBool(getStrFormat("radioLights[%d].has_shadow", index), shadow_enabled);
        if (shadow_enabled) {
            shader.setFloat(getStrFormat("radioLights[%d].inv_n", index), 1.0f / nearZ);
            shader.setFloat(getStrFormat("radioLights[%d].inv_f", index), 1.0f / farZ);
            shader.setTextureSource(getStrFormat("radioLights[%d].shadowMap", index), sm_index++, shadowMap->smBuffer->colorAttachs[0].texture->id, shadowMap->smBuffer->colorAttachs[0].texture->target);
            shader.setFloat(getStrFormat("radioLights[%d].resolution", index), (float)shadowMap->width);
            shader.setFloat(getStrFormat("radioLights[%d].light_size", index), light_size);
        }
    }
    void addGui(nanogui::FormHelper* gui, nanogui::ref<nanogui::Window> sceneWindow) {
        BaseLight::addGui(gui);
        gui->addGroup("Close");
        gui->addButton("Close", [this]() { lightWindow->setVisible(false); })->setIcon(ENTYPO_ICON_CROSS);
        gui->setWindow(sceneWindow);
    }
};