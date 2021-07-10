#pragma once

#include "renderpass.h"

Texture* createCubeMapFromTex2D(
	Texture* tex2D,
    FrameBuffer* targetFbo,
    uint width
) {
    checkEnvmapVAO();

    Texture* tex = Texture::createCubeMap(width, width, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);
    targetFbo->attachCubeTarget(tex, 0);

    glViewport(0, 0, width, width);
    Shader* shader = sManager.getShader(SID_DEFERRED_ENVMAP2D);
    shader->use();
    shader->setTextureSource("envmap", 0, tex2D->id, tex2D->target);
    targetFbo->prepare();
    glBindVertexArray(_envmap_vao);
    for (unsigned int i = 0; i < 6; ++i)
    {
        shader->setMat4("camera_vp", _capture_projection * _capture_views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tex->id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);
    frame_record.triangles += 72;
    frame_record.draw_calls += 6;
    return tex;
}

Texture* filterCubeMap(
    Texture* texCube,
    FrameBuffer* targetFbo,
    uint width
) {
    checkEnvmapVAO();

    Texture *tex = Texture::createCubeMap(width, width, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);
    targetFbo->attachCubeTarget(tex, 0);

    glViewport(0, 0, width, width);
    Shader* shader = sManager.getShader(SID_IBL_CONVOLUTION);
    shader->use();
    shader->setTextureSource("envmap", 0, texCube->id, texCube->target);
    targetFbo->prepare();
    glBindVertexArray(_envmap_vao);
    for (unsigned int i = 0; i < 6; ++i)
    {
        shader->setMat4("camera_vp", _capture_projection * _capture_views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tex->id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);
    frame_record.triangles += 72;
    frame_record.draw_calls += 6;
    return tex;
}

Texture* filterCubeMap(
    Texture* texCube,
    FrameBuffer* targetFbo,
    uint width,
    uint level
) {
    checkEnvmapVAO();

    Texture* tex = Texture::createCubeMap(width, width, GL_RGB32F, GL_RGB, GL_FLOAT, GL_LINEAR);
    tex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    tex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
    tex->setTexParami(GL_TEXTURE_MAX_LEVEL, level - 1);
    tex->genMipmap();
    targetFbo->attachCubeTarget(tex, 0);

    Shader* shader = sManager.getShader(SID_IBL_PREFILTER);
    shader->use();
    shader->setTextureSource("envmap", 0, texCube->id, texCube->target);
    shader->setFloat("resolution", (float)texCube->width);
    targetFbo->prepare();
    glBindVertexArray(_envmap_vao);
    for (uint mip = 0; mip < level; mip++) {
        glViewport(0, 0, width, width);
        float roughness = (float)mip / (float)(level - 1);
        shader->setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            shader->setMat4("camera_vp", _capture_projection * _capture_views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tex->id, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        width /= 2;
    }
    glBindVertexArray(0);
    frame_record.triangles += 72 * level;
    frame_record.draw_calls += 6 * level;
    return tex;
}

class IBL {
public:
    std::string name;
    glm::vec3 intensity;
    glm::vec3 position;
    float range;
    float miplevel;

    Texture* tex2D;
    Texture* texCube;
    Texture* texCubeFiltered;
    //Texture* texCubeFiltered2;
    FrameBuffer* targetFbo;

    IBL(std::string Path, glm::vec3 Intensity = glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f), float Range = 15.0f) {
        name = Path;
        intensity = Intensity;
        position = Position;
        range = Range;
        
        tex2D = Texture::createHDRMapFromFile(Path, true);
        targetFbo = new FrameBuffer;
        texCube = createCubeMapFromTex2D(tex2D, targetFbo, 512);
        texCube->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        texCube->genMipmap();
        texCubeFiltered = filterCubeMap(texCube, targetFbo, 512, 5);
        //texCubeFiltered2 = filterCubeMap(texCube, targetFbo, 32);
        miplevel = 4.0f;
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