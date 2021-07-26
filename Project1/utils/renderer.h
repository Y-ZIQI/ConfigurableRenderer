#pragma once

#include "renderpass.h"

class DeferredRenderer {
public:
    ScreenPass prePass;
    ScreenPass shadingPass;
    ScreenPass ssrPass;
    ScreenPass resolvePass;
    ScreenPass blurPass;
    ScreenPass joinPass;
    FrameBuffer* gPreBuffer;
    FrameBuffer* gBuffer;
    FrameBuffer* shadingBuffer;
    FrameBuffer* effectBuffer;
    FrameBuffer* joinBuffer;
    FrameBuffer* rayTraceBuffer;
    FrameBuffer* reflectionBuffer;
    FrameBuffer* screenBuffer;
    uint width, height;

    RandomHemisphereSample ssao_sample;
    PoissonDiskSample shadow_sample;
    HammersleySample ssr_sample;

    uint ssr = 0, ibl = 0, effect = 0;

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
        /*ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        ntex->setTexParami(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        ntex->setTexParami(GL_TEXTURE_REDUCTION_MODE_ARB, GL_MAX);
        ntex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
        ntex->setTexParami(GL_TEXTURE_MAX_LEVEL, 4);*/
        gBuffer->attachColorTarget(ntex, 1);
        gBuffer->attachColorTarget(Texture::create(width, height, GL_R16F, GL_RED, GL_FLOAT), 2);
        shadingBuffer = new FrameBuffer;
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        effectBuffer = new FrameBuffer;
        effectBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        effectBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        joinBuffer = new FrameBuffer;
        joinBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        rayTraceBuffer = new FrameBuffer;
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 0);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 1);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 2);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 3);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 4);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 5);
        reflectionBuffer = new FrameBuffer;
        reflectionBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        reflectionBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        reflectionBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 2);
        screenBuffer = new FrameBuffer;
        screenBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);

        prePass.setShader(sManager.getShader(SID_DEFERRED_PREPROCESS));
        shadingPass.setShader(sManager.getShader(SID_DEFERRED_SHADING));
        ssrPass.setShader(sManager.getShader(SID_SSR));
        resolvePass.setShader(sManager.getShader(SID_SSR_RESOLVE));
        blurPass.setShader(sManager.getShader(SID_GAUSSIAN_BLUR));
        joinPass.setShader(sManager.getShader(SID_JOIN_EFFECTS));

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
    void renderShading(Scene& scene) {
        shadingPass.shader->use();
        shadingBuffer->prepare();
        setShadingUniforms(scene);
        shadingPass.render();
        frame_record.triangles += 2;
        frame_record.draw_calls += 1;
    }
    void renderReflection(Scene& scene) {
        glViewport(0, 0, width / 2, height / 2);
        ssrPass.shader->use();
        rayTraceBuffer->prepare();
        ssr_sample.setUniforms(ssrPass.shader, 8);
        setSSRUniforms(scene);
        ssrPass.render();

        glViewport(0, 0, width, height);
        resolvePass.shader->use();
        reflectionBuffer->prepare(0);
        scene.setCameraUniforms(*resolvePass.shader);
        resolvePass.shader->setInt("width", width);
        resolvePass.shader->setInt("height", height);
        resolvePass.shader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("albedoTex", 1, gPreBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("specularTex", 2, gPreBuffer->colorAttachs[1].texture->id);
        resolvePass.shader->setTextureSource("positionTex", 3, gPreBuffer->colorAttachs[3].texture->id);
        resolvePass.shader->setTextureSource("normalTex", 4, gBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("hitPt12", 5, rayTraceBuffer->colorAttachs[0].texture->id);
        resolvePass.shader->setTextureSource("hitPt34", 6, rayTraceBuffer->colorAttachs[1].texture->id);
        resolvePass.shader->setTextureSource("hitPt56", 7, rayTraceBuffer->colorAttachs[2].texture->id);
        resolvePass.shader->setTextureSource("hitPt78", 8, rayTraceBuffer->colorAttachs[3].texture->id);
        resolvePass.shader->setTextureSource("weight1234", 9, rayTraceBuffer->colorAttachs[4].texture->id);
        resolvePass.shader->setTextureSource("weight5678", 10, rayTraceBuffer->colorAttachs[5].texture->id);
        resolvePass.render();

        blurPass.shader->use();
        reflectionBuffer->prepare(1);
        blurPass.shader->setTextureSource("colorTex", 0, reflectionBuffer->colorAttachs[0].texture->id);
        blurPass.shader->setBool("horizontal", true);
        blurPass.render();
        reflectionBuffer->prepare(2);
        blurPass.shader->setTextureSource("colorTex", 0, reflectionBuffer->colorAttachs[0].texture->id);
        blurPass.shader->setTextureSource("horizontalTex", 1, reflectionBuffer->colorAttachs[1].texture->id);
        blurPass.shader->setBool("horizontal", false);
        blurPass.render();

        joinPass.shader->use();
        reflectionBuffer->prepare(0);
        joinPass.shader->setBool("join", true);
        joinPass.shader->setBool("tone_mapping", false);
        joinPass.shader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        joinPass.shader->setTextureSource("joinTex", 1, reflectionBuffer->colorAttachs[2].texture->id);
        joinPass.render();

        frame_record.triangles += 8;
        frame_record.draw_calls += 4;
    }
    void renderEffects() {
        blurPass.shader->use();
        blurPass.shader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[1].texture->id);
        blurPass.shader->setTextureSource("horizontalTex", 1, effectBuffer->colorAttachs[0].texture->id);
        effectBuffer->prepare(0);
        blurPass.shader->setBool("horizontal", true);
        blurPass.render();

        effectBuffer->prepare(1);
        blurPass.shader->setBool("horizontal", false);
        blurPass.render();
    }
    void joinEffects(Texture *colorTex) {
        joinPass.shader->use();
        screenBuffer->prepare();
        joinPass.shader->setBool("join", effect != 0);
        joinPass.shader->setBool("tone_mapping", true);
        joinPass.shader->setTextureSource("colorTex", 0, colorTex->id);
        joinPass.shader->setTextureSource("joinTex", 1, effectBuffer->colorAttachs[1].texture->id);
        joinPass.render();
    }
    void renderScene(Scene& scene) {
        glViewport(0, 0, width, height);
        renderGeometry(scene);
        renderShading(scene);
        if(effect)
            renderEffects();
        if (ssr) {
            renderReflection(scene);
            joinEffects(reflectionBuffer->colorAttachs[0].texture);
        }
        else {
            joinEffects(shadingBuffer->colorAttachs[0].texture);
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
        scene.setCameraUniforms(*ssrPass.shader);
        ssrPass.shader->setInt("width", width);
        ssrPass.shader->setInt("height", height);
        ssrPass.shader->setTextureSource("specularTex", 1, gPreBuffer->colorAttachs[1].texture->id);
        ssrPass.shader->setTextureSource("positionTex", 2, gPreBuffer->colorAttachs[3].texture->id);
        ssrPass.shader->setTextureSource("normalTex", 3, gBuffer->colorAttachs[0].texture->id);
        ssrPass.shader->setTextureSource("lineardepthTex", 4, gBuffer->colorAttachs[1].texture->id);
        ssrPass.shader->setTextureSource("depthTex", 5, gPreBuffer->depthAttach.texture->id);
    }
};