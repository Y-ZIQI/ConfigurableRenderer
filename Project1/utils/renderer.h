#pragma once
#include "scene.h"
#include "framebuffer.h"
#include "sample.h"

class DeferredRenderer {
public:
    Shader* preShader;
    Shader* ssaoblurShader;
    Shader* shadingShader;
    Shader* ssrShader;
    Shader* resolveShader;
    Shader* ssrblurShader;
    Shader* blurShader;
    Shader* joinShader;
    FrameBuffer* gPreBuffer;
    FrameBuffer* gBuffer;
    FrameBuffer* aoBuffer;
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

    uint ssao = 0, ssr = 0, ibl = 0, effect = 0;

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
        aoBuffer = new FrameBuffer;
        aoBuffer->attachColorTarget(Texture::create(width, height, GL_R16F, GL_RED, GL_FLOAT), 0);
        shadingBuffer = new FrameBuffer;
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        effectBuffer = new FrameBuffer;
        effectBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        effectBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        joinBuffer = new FrameBuffer;
        joinBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        rayTraceBuffer = new FrameBuffer;
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 0);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 1);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 2);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 3);
        reflectionBuffer = new FrameBuffer;
        reflectionBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        reflectionBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        reflectionBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 2);
        screenBuffer = new FrameBuffer;
        screenBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);

        preShader = sManager.getShader(SID_DEFERRED_PREPROCESS);
        ssaoblurShader = sManager.getShader(SID_SSAO_FILTER);
        shadingShader = sManager.getShader(SID_DEFERRED_SHADING);
        ssrShader = sManager.getShader(SID_SSR_RAYTRACE);
        resolveShader = sManager.getShader(SID_SSR_RESOLVE);
        blurShader = sManager.getShader(SID_BLOOM_BLUR);
        ssrblurShader = sManager.getShader(SID_SSR_BLUR);
        joinShader = sManager.getShader(SID_JOIN_EFFECTS);

        ssao_sample.init(1.0, 32);
        shadow_sample.init(1.0, 5.0, 40, true);
        ssr_sample.init(8);
    }
    void renderGeometry(Scene& scene) {
        Shader* gShader = sManager.getShader(SID_DEFERRED_BASE);
        gShader->use();
        gPreBuffer->prepare();
        gPreBuffer->clear();
        scene.setCameraUniforms(*gShader);
        scene.Draw(*gShader, 0);
        renderEnvmap(scene);

        preShader->use();
        gBuffer->prepare();
        scene.setCameraUniforms(*preShader);
        if (ssao) {
            ssao_sample.setUniforms(preShader, 16);
            preShader->setFloat("ssao_range", scene.ssao_range);
            preShader->setFloat("ssao_bias", scene.ssao_bias);
            preShader->setFloat("ssao_threshold", scene.ssao_threshold);
        }
        preShader->setTextureSource("positionTex", 0, gPreBuffer->colorAttachs[3].texture->id);
        preShader->setTextureSource("tangentTex", 1, gPreBuffer->colorAttachs[4].texture->id);
        preShader->setTextureSource("normalTex", 2, gPreBuffer->colorAttachs[5].texture->id);
        preShader->setTextureSource("depthTex", 3, gPreBuffer->depthAttach.texture->id);
        renderScreen();
        //gBuffer->colorAttachs[1].texture->genMipmap();

        if (ssao) {
            ssaoblurShader->use();
            ssaoblurShader->setInt("ksize", 9);
            aoBuffer->prepare();
            ssaoblurShader->setTextureSource("colorTex", 0, gBuffer->colorAttachs[2].texture->id);
            ssaoblurShader->setBool("horizontal", true);
            renderScreen();
            gBuffer->prepare(2);
            ssaoblurShader->setTextureSource("horizontalTex", 1, aoBuffer->colorAttachs[0].texture->id);
            ssaoblurShader->setBool("horizontal", false);
            renderScreen();
        }
    }
    void renderEnvmap(Scene& scene) {
        if (!scene.envMap_enabled || scene.envIdx < 0) return;
        gPreBuffer->prepare(0);
        scene.DrawEnvMap(*sManager.getShader(SID_DEFERRED_ENVMAP));
    }
    void renderShading(Scene& scene) {
        shadingShader->use();
        shadingBuffer->prepare();
        setShadingUniforms(scene);
        renderScreen();
    }
    void renderReflection(Scene& scene) {
        glViewport(0, 0, width / 2, height / 2);
        ssrShader->use();
        rayTraceBuffer->prepare();
        ssr_sample.setUniforms(ssrShader, 8);
        setSSRUniforms(scene);
        renderScreen();

        glViewport(0, 0, width, height);
        resolveShader->use();
        reflectionBuffer->prepare(0);
        scene.setCameraUniforms(*resolveShader);
        resolveShader->setInt("width", width);
        resolveShader->setInt("height", height);
        resolveShader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("albedoTex", 1, gPreBuffer->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("specularTex", 2, gPreBuffer->colorAttachs[1].texture->id);
        resolveShader->setTextureSource("positionTex", 3, gPreBuffer->colorAttachs[3].texture->id);
        resolveShader->setTextureSource("normalTex", 4, gBuffer->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("hitPt1234", 5, rayTraceBuffer->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("hitPt5678", 6, rayTraceBuffer->colorAttachs[1].texture->id);
        resolveShader->setTextureSource("weight1234", 7, rayTraceBuffer->colorAttachs[2].texture->id);
        resolveShader->setTextureSource("weight5678", 8, rayTraceBuffer->colorAttachs[3].texture->id);
        renderScreen();

        ssrblurShader->use();
        reflectionBuffer->prepare(1);
        ssrblurShader->setTextureSource("colorTex", 0, reflectionBuffer->colorAttachs[0].texture->id);
        ssrblurShader->setTextureSource("specularTex", 2, gPreBuffer->colorAttachs[1].texture->id);
        ssrblurShader->setBool("horizontal", true);
        renderScreen();
        reflectionBuffer->prepare(2);
        ssrblurShader->setTextureSource("horizontalTex", 1, reflectionBuffer->colorAttachs[1].texture->id);
        ssrblurShader->setBool("horizontal", false);
        renderScreen();

        joinShader->use();
        reflectionBuffer->prepare(0);
        joinShader->setBool("join", true);
        joinShader->setBool("substract", false);
        joinShader->setBool("tone_mapping", false);
        joinShader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        joinShader->setTextureSource("joinTex", 1, reflectionBuffer->colorAttachs[2].texture->id);
        renderScreen();
    }
    void renderEffects() {
        blurShader->use();
        blurShader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[1].texture->id);
        blurShader->setTextureSource("horizontalTex", 1, effectBuffer->colorAttachs[0].texture->id);
        effectBuffer->prepare(0);
        blurShader->setBool("horizontal", true);
        renderScreen();

        effectBuffer->prepare(1);
        blurShader->setBool("horizontal", false);
        renderScreen();
    }
    void joinEffects(Texture *colorTex) {
        joinShader->use();
        screenBuffer->prepare();
        joinShader->setBool("join", effect != 0);
        joinShader->setBool("substract", effect != 0);
        joinShader->setBool("tone_mapping", true);
        joinShader->setTextureSource("colorTex", 0, colorTex->id);
        joinShader->setTextureSource("joinTex", 1, effectBuffer->colorAttachs[1].texture->id);
        renderScreen();
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
        scene.setCameraUniforms(*shadingShader);
        scene.setLightUniforms(*shadingShader, ibl != 0);
        shadow_sample.setUniforms(shadingShader, 40);
        shadingShader->setTextureSource("albedoTex", 0, gPreBuffer->colorAttachs[0].texture->id);
        shadingShader->setTextureSource("specularTex", 1, gPreBuffer->colorAttachs[1].texture->id);
        shadingShader->setTextureSource("emissiveTex", 2, gPreBuffer->colorAttachs[2].texture->id);
        shadingShader->setTextureSource("positionTex", 3, gPreBuffer->colorAttachs[3].texture->id);
        shadingShader->setTextureSource("normalTex", 4, gBuffer->colorAttachs[0].texture->id);
        shadingShader->setTextureSource("depthTex", 5, gBuffer->colorAttachs[1].texture->id);
        shadingShader->setTextureSource("aoTex", 6, gBuffer->colorAttachs[2].texture->id);
    }
    void setSSRUniforms(Scene& scene) {
        scene.setCameraUniforms(*ssrShader);
        ssrShader->setFloat("threshold", scene.ssr_threshold);
        ssrShader->setInt("width", width);
        ssrShader->setInt("height", height);
        ssrShader->setTextureSource("specularTex", 1, gPreBuffer->colorAttachs[1].texture->id);
        ssrShader->setTextureSource("positionTex", 2, gPreBuffer->colorAttachs[3].texture->id);
        ssrShader->setTextureSource("normalTex", 3, gBuffer->colorAttachs[0].texture->id);
        ssrShader->setTextureSource("lineardepthTex", 4, gBuffer->colorAttachs[1].texture->id);
        ssrShader->setTextureSource("depthTex", 5, gPreBuffer->depthAttach.texture->id);
    }
};