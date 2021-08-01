#pragma once
#include "scene.h"
#include "framebuffer.h"

class OmnidirectionalPass {
public:
    Shader* cubeShader;
    FrameBuffer* targetFbo;
    FrameBuffer* readFbo;
    FrameBuffer* drawFbo;
    uint width, height;
    glm::vec3 position;
    glm::mat4 projMat;
    std::vector<glm::mat4> transforms;

    OmnidirectionalPass(uint Width, uint Height) {
        width = Width; height = Height;
        targetFbo = new FrameBuffer;
        targetFbo->setParami(GL_FRAMEBUFFER_DEFAULT_LAYERS, 6);
        readFbo = new FrameBuffer;
        drawFbo = new FrameBuffer;
        transforms.resize(6);
        cubeShader = sManager.getShader(SID_CUBEMAP_RENDER);
    }
    void setTarget(Texture* cudeTarget, Texture* depthTarget = nullptr) {
        targetFbo->attachCubeTarget(cudeTarget, 0);
        if(depthTarget)
            targetFbo->attachCubeDepthTarget(depthTarget);
    }
    void setTransforms(glm::vec3 pos, glm::mat4 proj) {
        position = pos;
        projMat = proj;
        transforms[0] = proj * glm::lookAt(pos, pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms[1] = proj * glm::lookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms[2] = proj * glm::lookAt(pos, pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        transforms[3] = proj * glm::lookAt(pos, pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        transforms[4] = proj * glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        transforms[5] = proj * glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    }
    void blitEnvmap(Texture* envmap) {
        readFbo->use();
        for(uint i = 0;i < 6;i++)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envmap->id, 0);
        drawFbo->use();
        for (uint i = 0; i < 6; i++)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, targetFbo->colorAttachs[0].texture->id, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo->id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo->id);
        for (uint i = 0; i < 6; i++) {
            glReadBuffer(_color_attachments[i]);
            glDrawBuffer(_color_attachments[i]);
            glBlitFramebuffer(0, 0, envmap->width, envmap->height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }
    void renderScene(Scene& scene) {
        glViewport(0, 0, width, height);
        cubeShader->use();
        setUniforms();
        scene.setLightUniforms(*cubeShader, false);
        targetFbo->prepare();
        targetFbo->clear(DEPTH_TARGETS);
        scene.Draw(*cubeShader, 1);
        targetFbo->colorAttachs[0].texture->genMipmap();
    }
    void setUniforms() {
        cubeShader->setVec3("camera_pos", position);
        cubeShader->setMat4("transforms[0]", transforms[0]);
        cubeShader->setMat4("transforms[1]", transforms[1]);
        cubeShader->setMat4("transforms[2]", transforms[2]);
        cubeShader->setMat4("transforms[3]", transforms[3]);
        cubeShader->setMat4("transforms[4]", transforms[4]);
        cubeShader->setMat4("transforms[5]", transforms[5]);
    }
};
