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

class DeferredRenderer {
public:
    ScreenPass prePass;
    ScreenPass shadingPass;
    ScreenPass ssrPass;
    ScreenPass resolvePass;
    FrameBuffer* gPreBuffer;
    FrameBuffer* gBuffer;
    FrameBuffer* shadingBuffer;
    FrameBuffer* reflectionBuffer;
    FrameBuffer* screenBuffer;
    uint width, height;

    RandomHemisphereSample ssao_sample;
    PoissonDiskSample shadow_sample;
    HammersleySample ssr_sample;

    uint ssr = 0, ibl = 0;

    TimeRecord record[2]; // Shading, AO

    DeferredRenderer(uint Width, uint Height) {
        width = Width; height = Height;
        gPreBuffer = new FrameBuffer;
        gPreBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), 0);
        gPreBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 1);
        gPreBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 2);
        gPreBuffer->attachColorTarget(Texture::create(width, height, GL_RGB32F, GL_RGB, GL_FLOAT), 3);
        gPreBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT), 4);
        gPreBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT), 5);
        gPreBuffer->attachDepthTarget(Texture::create(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR));
        gBuffer = new FrameBuffer;
        gBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        Texture* ntex = Texture::create(width, height, GL_R32F, GL_RED, GL_FLOAT, GL_NEAREST);
        gBuffer->attachColorTarget(ntex, 1);
        /*ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        ntex->setTexParami(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        ntex->setTexParami(GL_TEXTURE_REDUCTION_MODE_ARB, GL_MAX);
        ntex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
        ntex->setTexParami(GL_TEXTURE_MAX_LEVEL, 4);*/
        gBuffer->attachColorTarget(Texture::create(width, height, GL_R16F, GL_RED, GL_FLOAT), 2);
        shadingBuffer = new FrameBuffer;
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 0);
        reflectionBuffer = new FrameBuffer;
        //reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST), 0);
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST), 1);
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST), 2);
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST), 3);
        screenBuffer = new FrameBuffer;
        screenBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);

        prePass.setShader(sManager.getShader(SID_DEFERRED_PREPROCESS));
        shadingPass.setShader(sManager.getShader(SID_DEFERRED_SHADING));
        ssrPass.setShader(sManager.getShader(SID_SSR));
        resolvePass.setShader(sManager.getShader(SID_SSR_RESOLVE));

        ssao_sample.init(1.0, 32);
        shadow_sample.init(1.0, 5.0, 40, true);
        ssr_sample.init(8);
    }
    void renderGeometry(Scene& scene) {
        Shader* gShader = sManager.getShader(SID_DEFERRED_BASE);
        gShader->use();
        gPreBuffer->clear();
        gPreBuffer->prepare();
        scene.setCameraUniforms(*gShader);
        scene.Draw(*gShader, 0);
        renderEnvmap(scene);

        prePass.shader->use();
        gBuffer->prepare();
        ssao_sample.setUniforms(prePass.shader, 16);
        scene.setCameraUniforms(*prePass.shader);
        prePass.shader->setTextureSource("positionTex", 0, gPreBuffer->colorAttachs[3].texture->id);
        prePass.shader->setTextureSource("tangentTex", 1, gPreBuffer->colorAttachs[4].texture->id);
        prePass.shader->setTextureSource("normalTex", 2, gPreBuffer->colorAttachs[5].texture->id);
        prePass.shader->setTextureSource("depthTex", 3, gPreBuffer->depthAttach.texture->id);
        prePass.render();
        //gBuffer->colorAttachs[1].texture->genMipmap();
        frame_record.triangles += 2;
        frame_record.draw_calls += 1;
    }
    void renderEnvmap(Scene& scene) {
        if (!scene.envMap_enabled || scene.envIdx < 0)
            return;
        Texture* envmap = scene.envMaps[scene.envIdx];
        bool is_cube = envmap->target == GL_TEXTURE_CUBE_MAP;
        Shader* eShader = sManager.getShader(is_cube ? SID_DEFERRED_ENVMAP : SID_DEFERRED_ENVMAP2D);
        eShader->use();
        gPreBuffer->prepare(0);
        scene.setCameraUniforms(*eShader, true);
        eShader->setTextureSource("envmap", 0, envmap->id, envmap->target);
        scene.DrawEnvMap(*eShader);
        gPreBuffer->prepare();
    }
    void renderShading(Scene& scene, FrameBuffer* outBuffer) {
        shadingPass.shader->use();
        outBuffer->prepare();
        setShadingUniforms(scene);
        shadingPass.render();
        frame_record.triangles += 2;
        frame_record.draw_calls += 1;
    }
    void renderReflection(Scene& scene, FrameBuffer* outBuffer) {
        glViewport(0, 0, width / 2, height / 2);
        ssrPass.shader->use();
        reflectionBuffer->prepare();
        ssr_sample.setUniforms(ssrPass.shader, 8);
        setSSRUniforms(scene);
        ssrPass.render();

        glViewport(0, 0, width, height);
        resolvePass.shader->use();
        outBuffer->prepare();
        /*resolvePass.shader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("reflectionTex", 1, reflectionBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("specularTex", 2, gPreBuffer->colorAttachs[1].texture->id);*/
        scene.setCameraUniforms(*resolvePass.shader);
        resolvePass.shader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("albedoTex", 1, gPreBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("specularTex", 2, gPreBuffer->colorAttachs[1].texture->id);
        resolvePass.shader->setTextureSource("positionTex", 3, gPreBuffer->colorAttachs[3].texture->id);
        resolvePass.shader->setTextureSource("normalTex", 4, gBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("hitPt12", 5, reflectionBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("hitPt34", 6, reflectionBuffer->colorAttachs[1].texture->id);
        resolvePass.shader->setTextureSource("hitPt56", 7, reflectionBuffer->colorAttachs[2].texture->id);
        resolvePass.shader->setTextureSource("hitPt78", 8, reflectionBuffer->colorAttachs[3].texture->id);
        resolvePass.render();
        frame_record.triangles += 2;
        frame_record.draw_calls += 1;
    }
    void renderScene(Scene& scene) {
        glViewport(0, 0, width, height);
        renderGeometry(scene);
        if (ssr) {
            renderShading(scene, shadingBuffer);
            renderReflection(scene, screenBuffer);
        }
        else {
            renderShading(scene, screenBuffer);
        }
    }
    void setShadingUniforms(Scene& scene) {
        scene.setCameraUniforms(*shadingPass.shader);
        scene.setLightUniforms(*shadingPass.shader, ibl != 0); 
        shadow_sample.setUniforms(shadingPass.shader, 40);
        shadingPass.shader->setTextureSource("albedoTex", 0, gPreBuffer->colorAttachs[0].texture->id);
        shadingPass.shader->setTextureSource("specularTex", 1, gPreBuffer->colorAttachs[1].texture->id);
        shadingPass.shader->setTextureSource("emissiveTex", 2, gPreBuffer->colorAttachs[2].texture->id);
        shadingPass.shader->setTextureSource("positionTex", 3, gPreBuffer->colorAttachs[3].texture->id);
        shadingPass.shader->setTextureSource("normalTex", 4, gBuffer->colorAttachs[0].texture->id);
        shadingPass.shader->setTextureSource("depthTex", 5, gBuffer->colorAttachs[1].texture->id);
        shadingPass.shader->setTextureSource("aoTex", 6, gBuffer->colorAttachs[2].texture->id);
    }
    void setSSRUniforms(Scene& scene) {
        /*scene.setCameraUniforms(*ssrPass.shader);
        ssrPass.shader->setTextureSource("albedoTex", 0, gPreBuffer->colorAttachs[0].texture->id);
        ssrPass.shader->setTextureSource("specularTex", 1, gPreBuffer->colorAttachs[1].texture->id);
        ssrPass.shader->setTextureSource("positionTex", 2, gPreBuffer->colorAttachs[3].texture->id);
        ssrPass.shader->setTextureSource("normalTex", 3, gBuffer->colorAttachs[0].texture->id);
        ssrPass.shader->setTextureSource("lineardepthTex", 4, gBuffer->colorAttachs[1].texture->id);
        ssrPass.shader->setTextureSource("depthTex", 5, gPreBuffer->depthAttach.texture->id);
        ssrPass.shader->setTextureSource("colorTex", 6, shadingBuffer->colorAttachs[0].texture->id);*/

        scene.setCameraUniforms(*ssrPass.shader);
        ssrPass.shader->setTextureSource("specularTex", 1, gPreBuffer->colorAttachs[1].texture->id);
        ssrPass.shader->setTextureSource("positionTex", 2, gPreBuffer->colorAttachs[3].texture->id);
        ssrPass.shader->setTextureSource("normalTex", 3, gBuffer->colorAttachs[0].texture->id);
        ssrPass.shader->setTextureSource("lineardepthTex", 4, gBuffer->colorAttachs[1].texture->id);
        ssrPass.shader->setTextureSource("depthTex", 5, gPreBuffer->depthAttach.texture->id);
    }
};