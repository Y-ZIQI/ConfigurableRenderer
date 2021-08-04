#pragma once

#include "framebuffer.h"
#include "shader.h"

Texture* createCubeMapFromTex2D(
	Texture* tex2D,
    FrameBuffer* targetFbo,
    uint width
) {
    Texture* tex = Texture::createCubeMap(width, width, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);

    glViewport(0, 0, width, width);
    Shader* shader = sManager.getShader(SID_DEFERRED_ENVMAP);
    shader->use();
    shader->setBool("target_2d", true);
    shader->setTextureSource("envmap", 0, 0, GL_TEXTURE_CUBE_MAP);
    shader->setTextureSource("envmap2d", 1, tex2D->id, GL_TEXTURE_2D);
    for (unsigned int i = 0; i < 6; ++i)
    {
        shader->setMat4("camera_vp", _capture_projection * _capture_views[i]);
        targetFbo->attachColorTarget(tex, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
        targetFbo->prepare();
        targetFbo->clear();
        renderCube();
    }
    return tex;
}

void filterCubeMap(
    Texture* targetTex,
    Texture* texCube,
    FrameBuffer* targetFbo,
    uint width,
    uint level
) {
    Shader* shader = sManager.getShader(SID_IBL_PREFILTER);
    shader->use();
    shader->setTextureSource("envmap", 0, texCube->id, texCube->target);
    shader->setFloat("resolution", (float)texCube->width);
    for (uint mip = 0; mip < level; mip++) {
        glViewport(0, 0, width, width);
        float roughness = (float)mip / (float)(level - 1);
        shader->setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            shader->setMat4("camera_vp", _capture_projection * _capture_views[i]);
            targetFbo->attachColorTarget(targetTex, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
            targetFbo->prepare();
            targetFbo->clear();
            renderCube();
        }
        width /= 2;
    }
}

class IBL {
public:
    std::string name;
    std::string path;
    glm::vec3 intensity;
    glm::vec3 position;
    float range;
    float nearz, farz;
    float miplevel;
    // Lighting images
    bool processed;
    Texture* tex2D = nullptr;
    Texture* texCube = nullptr;
    Texture* depthCube = nullptr;
    Texture* texCubeFiltered = nullptr;
    FrameBuffer* targetFbo = nullptr;
    // For GUI
    nanogui::ref<nanogui::Window> lightWindow;

    IBL(
        std::string Name, 
        std::string Path, 
        glm::vec3 Intensity = glm::vec3(1.0f, 1.0f, 1.0f), 
        glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f), 
        float Range = 15.0f, float Nearz = 0.1f, float Farz = 100.0f
    ) {
        name = Name;
        path = Path;
        intensity = Intensity;
        position = Position;
        range = Range;
        nearz = Nearz;
        farz = Farz;
        
        if (Path != "") {
            tex2D = Texture::createHDRMapFromFile(Path, true);
            targetFbo = new FrameBuffer;
            texCube = createCubeMapFromTex2D(tex2D, targetFbo, 512);
            texCube->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            texCube->genMipmap();
            filter(5);
            processed = true;
        }
        else {
            processed = false;
            targetFbo = new FrameBuffer;
            texCube = Texture::createCubeMap(512, 512, GL_RGB16F, GL_RGB, GL_FLOAT, GL_LINEAR);
            texCube->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            depthCube = Texture::createCubeMap(512, 512, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR);
        }
    }
    void filter(uint level = 5) {
        if(texCubeFiltered == nullptr){
            texCubeFiltered = Texture::createCubeMap(512, 512, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);
            texCubeFiltered->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            texCubeFiltered->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
            texCubeFiltered->setTexParami(GL_TEXTURE_MAX_LEVEL, level - 1);
            texCubeFiltered->genMipmap();
        }
        filterCubeMap(texCubeFiltered, texCube, targetFbo, 512, level);
        miplevel = (float)(level - 1);
    }
    void setUniforms(Shader& shader, uint index, uint& tex_index) {
        shader.setVec3(getStrFormat("ibls[%d].intensity", index), intensity);
        shader.setVec3(getStrFormat("ibls[%d].position", index), position);
        shader.setFloat(getStrFormat("ibls[%d].range", index), range);
        shader.setFloat(getStrFormat("ibls[%d].miplevel", index), miplevel);
        shader.setTextureSource(getStrFormat("ibls[%d].prefilterMap", index), tex_index++, texCubeFiltered->id, texCubeFiltered->target);
    }
    void addGui(nanogui::FormHelper* gui, nanogui::ref<nanogui::Window> sceneWindow) {
        gui->addButton(name, [this]() {
            lightWindow->setFocused(!lightWindow->visible());
            lightWindow->setVisible(!lightWindow->visible());
        })->setIcon(ENTYPO_ICON_IMAGE);
        lightWindow = gui->addWindow(Eigen::Vector2i(500, 0), name);
        lightWindow->setWidth(250);
        lightWindow->setVisible(false);
        gui->addGroup("Intensity");
        gui->addVariable("Intensity.r", intensity[0])->setSpinnable(true);
        gui->addVariable("Intensity.g", intensity[1])->setSpinnable(true);
        gui->addVariable("Intensity.b", intensity[2])->setSpinnable(true);
        gui->addGroup("Position");
        gui->addVariable("Position.x", position[0], false);
        gui->addVariable("Position.y", position[1], false);
        gui->addVariable("Position.z", position[2], false);
        gui->addGroup("Paramaters");
        gui->addVariable("Range", range)->setSpinnable(true);
        gui->addVariable("Near", nearz, false);
        gui->addVariable("Far", farz, false);
        gui->addGroup("Close");
        gui->addButton("Close", [this]() { lightWindow->setVisible(false); })->setIcon(ENTYPO_ICON_CROSS);
        gui->setWindow(sceneWindow);
    }
};
