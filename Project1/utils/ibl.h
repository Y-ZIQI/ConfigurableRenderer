#pragma once

#include "renderpass.h"

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
    frame_record.triangles += 72;
    frame_record.draw_calls += 6;
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
    frame_record.triangles += 72 * level;
    frame_record.draw_calls += 6 * level;
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

    bool processed;
    Texture* tex2D = nullptr;
    Texture* texCube = nullptr;
    Texture* depthCube = nullptr;
    Texture* texCubeFiltered = nullptr;
    //Texture* texCubeFiltered2;
    FrameBuffer* targetFbo = nullptr;

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
        //texCubeFiltered = filterCubeMap(texCube, targetFbo, 512, 5);
        //texCubeFiltered2 = filterCubeMap(texCube, targetFbo, 32);
        //miplevel = 4.0f;
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
        char tmp[64];
        sprintf(tmp, "ibls[%d].intensity", index);
        shader.setVec3(tmp, intensity);
        sprintf(tmp, "ibls[%d].position", index);
        shader.setVec3(tmp, position);
        sprintf(tmp, "ibls[%d].range", index);
        shader.setFloat(tmp, range);
        sprintf(tmp, "ibls[%d].miplevel", index);
        shader.setFloat(tmp, miplevel);
        sprintf(tmp, "ibls[%d].prefilterMap", index);
        shader.setTextureSource(tmp, tex_index++, texCubeFiltered->id, texCubeFiltered->target);
    }
};
