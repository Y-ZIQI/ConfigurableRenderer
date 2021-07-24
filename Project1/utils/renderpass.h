#pragma once
#include "scene.h"
#include "camera.h"
#include "framebuffer.h"
#include "sample.h"

class RenderPass {
public:
    Shader* shader;

    RenderPass() {};
    void setShader(Shader* Shader) {
        this->shader = Shader;
    };
};

class ScreenPass : public RenderPass {
public:
    ScreenPass() {
        checkScreenVAO();
    }
    void render() {
        // TODO
        shader->use();
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(_screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};

class OmnidirectionalPass : public RenderPass {
public:
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
        setShader(sManager.getShader(SID_CUBEMAP_RENDER));
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
        shader->use();
        targetFbo->clear(DEPTH_TARGETS);
        targetFbo->prepare();
        setUniforms();
        scene.setLightUniforms(*shader, false);
        scene.Draw(*shader, 1);
        targetFbo->colorAttachs[0].texture->genMipmap();
    }
    void setUniforms() {
        shader->setVec3("camera_pos", position);
        shader->setMat4("transforms[0]", transforms[0]);
        shader->setMat4("transforms[1]", transforms[1]);
        shader->setMat4("transforms[2]", transforms[2]);
        shader->setMat4("transforms[3]", transforms[3]);
        shader->setMat4("transforms[4]", transforms[4]);
        shader->setMat4("transforms[5]", transforms[5]);
    }
};

/*class OmnidirectionalPass : public RenderPass {
public:
    FrameBuffer* cube;
    uint width, height;
    glm::vec3 position;
    glm::mat4 projMat;
    std::vector<glm::mat4> transforms;

    OmnidirectionalPass(uint Width, uint Height) {
        width = Width; height = Height;
        cube = new FrameBuffer;
        cube->attachCubeTarget(Texture::createCubeMap(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 0);
        cube->attachCubeDepthTarget(Texture::createCubeMap(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
        cube->setParami(GL_FRAMEBUFFER_DEFAULT_LAYERS, 6);
        bool is_success = cube->checkStatus();
        if (!is_success) {
            cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
            return;
        }
        transforms.resize(6);
        setShader(sManager.getShader(SID_CUBEMAP_RENDER));
    }
    void setTransforms(glm::vec3 pos, glm::mat4 proj) {
        position = pos;
        projMat = proj;
        transforms[0] = proj * glm::lookAt(pos, pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));	//+x
        transforms[1] = proj * glm::lookAt(pos, pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));//-x
        transforms[2] = proj * glm::lookAt(pos, pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));	//+y
        transforms[3] = proj * glm::lookAt(pos, pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));//-y
        transforms[4] = proj * glm::lookAt(pos, pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));	//+z
        transforms[5] = proj * glm::lookAt(pos, pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));//-z
    }
    void renderScene(Scene& scene, float resolution = 1.0f) {
        glViewport(0, 0, width * resolution, height * resolution);
        shader->use();
        cube->clear();
        cube->prepare();
        setBaseUniforms(scene);
        scene.Draw(*shader, 0);
    }
    void setBaseUniforms(Scene& scene) {
        shader->setMat4("transforms[0]", transforms[0]);
        shader->setMat4("transforms[1]", transforms[1]);
        shader->setMat4("transforms[2]", transforms[2]);
        shader->setMat4("transforms[3]", transforms[3]);
        shader->setMat4("transforms[4]", transforms[4]);
        shader->setMat4("transforms[5]", transforms[5]);
    }
};*/
