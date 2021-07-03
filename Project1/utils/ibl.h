#pragma once

#include "renderpass.h"

Texture* createCubeMapFromTex2D(
	Texture* tex2D,
    FrameBuffer* targetFbo,
    uint width
) {
    checkEnvmapVAO();

    Texture* tex = Texture::createCubeMap(width, width, GL_RGB16F, GL_RGB, GL_FLOAT, GL_LINEAR);
    targetFbo->attachCubeTarget(tex, 2);

    glViewport(0, 0, width, width);
    Shader* shader = sManager.getShader(SID_DEFERRED_ENVMAP2D);
    shader->use();
    shader->setTextureSource("envmap", 0, tex2D->id, tex2D->target);
    targetFbo->prepare();
    glBindVertexArray(_envmap_vao);
    for (unsigned int i = 0; i < 6; ++i)
    {
        shader->setMat4("camera_vp", _capture_projection * _capture_views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
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
    Texture* tex2D,
    FrameBuffer* targetFbo,
    uint width
) {
    Texture *tex = Texture::createCubeMap(width, width, GL_RGB16F, GL_RGB, GL_FLOAT, GL_LINEAR);
    targetFbo->attachCubeTarget(tex, 0);

    glViewport(0, 0, width, width);
    Shader* shader = sManager.getShader(SID_IBL_CONVOLUTION);
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

class IBL {
public:
    Texture* tex2D;
    Texture* texCube;
    Texture* texCubeFiltered;
    FrameBuffer* targetFbo;

    IBL(std::string path) {
        tex2D = Texture::createHDRMapFromFile(path, true);
        targetFbo = new FrameBuffer;
        texCube = createCubeMapFromTex2D(tex2D, targetFbo, 512);
        texCubeFiltered = filterCubeMap(texCube, targetFbo, 32);
    }
    void setUniforms(Shader& shader, uint index, uint tex_index) {
        char tmp[64];
        sprintf(tmp, "ibls[%d].irradianceMap", index);
        shader.setTextureSource(tmp, tex_index, texCubeFiltered->id, texCubeFiltered->target);
    }
};