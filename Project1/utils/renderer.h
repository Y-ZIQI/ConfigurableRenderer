#pragma once
#include "scene.h"
#include "framebuffer.h"
#include "sample.h"

class DeferredRenderer {
public:
    Shader* gShader;
    Shader* envmapShader;
    Shader* preShader;
    Shader* ssaoblurShader;
    Shader* shadingShader;
    Shader* ssrtilingShader;
    Shader* ssrShader;
    Shader* resolveShader;
    Shader* ssrblurShader;
    Shader* bloomShader;
    Shader* joinShader;
    FrameBuffer* gBuffer1;
    FrameBuffer* gBuffer2;
    FrameBuffer* aoBuffer;
    FrameBuffer* shadingBuffer;
    FrameBuffer* effectBuffer;
    FrameBuffer* tilingBuffer;
    FrameBuffer* rayTraceBuffer;
    FrameBuffer* reflectionBuffer;
    FrameBuffer* joinBuffer;
    FrameBuffer* screenBuffer;
    uint width, height;
    glm::vec2 pixel_size;

    RandomHemisphereSample ssao_sample;
    PoissonDiskSample shadow_sample;
    HammersleySample ssr_sample;

    uint ssao = 0, ssr = 0, ibl = 0, effect = 0;

    TimeRecord record[2]; // Shading, AO

    DeferredRenderer(uint Width, uint Height) {
        width = Width; height = Height;
        pixel_size = glm::vec2(1.0 / (float)Width, 1.0 / (float)Height);
        gBuffer1 = new FrameBuffer;
        gBuffer1->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), 0);
        gBuffer1->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 1);
        gBuffer1->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 2);
        gBuffer1->attachColorTarget(Texture::create(width, height, GL_RGB32F, GL_RGB, GL_FLOAT), 3);
        gBuffer1->attachColorTarget(Texture::create(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT), 4);
        gBuffer1->attachColorTarget(Texture::create(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT), 5);
        gBuffer1->attachDepthTarget(Texture::create(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR));
        gBuffer2 = new FrameBuffer;
        gBuffer2->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        Texture* ntex = Texture::create(width, height, GL_R32F, GL_RED, GL_FLOAT, GL_NEAREST);
        /*ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        ntex->setTexParami(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        ntex->setTexParami(GL_TEXTURE_REDUCTION_MODE_ARB, GL_MAX);
        ntex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
        ntex->setTexParami(GL_TEXTURE_MAX_LEVEL, 4);*/
        gBuffer2->attachColorTarget(ntex, 1);
        gBuffer2->attachColorTarget(Texture::create(width, height, GL_R16F, GL_RED, GL_FLOAT), 2);
        aoBuffer = new FrameBuffer;
        aoBuffer->attachColorTarget(Texture::create(width, height, GL_R16F, GL_RED, GL_FLOAT), 0);
        shadingBuffer = new FrameBuffer;
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        shadingBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        effectBuffer = new FrameBuffer;
        effectBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        effectBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        tilingBuffer = new FrameBuffer;
        tilingBuffer->attachColorTarget(Texture::create(width / 8, height / 8, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
        rayTraceBuffer = new FrameBuffer;
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 0);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_NEAREST), 1);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 2);
        rayTraceBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 3);
        reflectionBuffer = new FrameBuffer;
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGB16F, GL_RGB, GL_FLOAT), 1);
        reflectionBuffer->attachColorTarget(Texture::create(width / 2, height / 2, GL_RGB16F, GL_RGB, GL_FLOAT), 2);
        joinBuffer = new FrameBuffer;
        joinBuffer->attachColorTarget(Texture::create(width, height, GL_RGB16F, GL_RGB, GL_FLOAT), 0);
        screenBuffer = new FrameBuffer;
        screenBuffer->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);

        gShader = sManager.getShader(SID_DEFERRED_BASE);
        envmapShader = sManager.getShader(SID_DEFERRED_ENVMAP);
        preShader = sManager.getShader(SID_DEFERRED_PREPROCESS);
        ssaoblurShader = sManager.getShader(SID_SSAO_FILTER);
        shadingShader = sManager.getShader(SID_DEFERRED_SHADING);
        ssrtilingShader = sManager.getShader(SID_SSR_TILING);
        ssrShader = sManager.getShader(SID_SSR_RAYTRACE);
        resolveShader = sManager.getShader(SID_SSR_RESOLVE);
        ssrblurShader = sManager.getShader(SID_SSR_BLUR);
        bloomShader = sManager.getShader(SID_BLOOM_BLUR);
        joinShader = sManager.getShader(SID_JOIN_EFFECTS);

        ssao_sample.init(1.0, 32);
        shadow_sample.init(1.0, 5.0, 40, true);
        ssr_sample.init(8);
    }
    void renderGeometry(Scene& scene) {
        glViewport(0, 0, width, height);
        gShader->use();
        scene.setCameraUniforms(*gShader);
        gBuffer1->prepare();
        gBuffer1->clear();
        scene.Draw(*gShader, 0);
        if (!scene.envMap_enabled || scene.envIdx < 0) return;
        gBuffer1->prepare(0);
        scene.DrawEnvMap(*envmapShader);

        preShader->use();
        scene.setCameraUniforms(*preShader);
        if (ssao) {
            ssao_sample.setUniforms(preShader, 16);
            preShader->setFloat("ssao_range", scene.ssao_range);
            preShader->setFloat("ssao_bias", scene.ssao_bias);
            preShader->setFloat("ssao_threshold", scene.ssao_threshold);
        }
        preShader->setTextureSource("positionTex", 0, gBuffer1->colorAttachs[3].texture->id);
        preShader->setTextureSource("tangentTex", 1, gBuffer1->colorAttachs[4].texture->id);
        preShader->setTextureSource("normalTex", 2, gBuffer1->colorAttachs[5].texture->id);
        preShader->setTextureSource("depthTex", 3, gBuffer1->depthAttach.texture->id);
        gBuffer2->prepare();
        renderScreen();
        //gBuffer2->colorAttachs[1].texture->genMipmap();

        if (ssao) {
            ssaoblurShader->use();
            ssaoblurShader->setInt("ksize", 9);
            ssaoblurShader->setTextureSource("colorTex", 0, gBuffer2->colorAttachs[2].texture->id);
            ssaoblurShader->setBool("horizontal", true);
            aoBuffer->prepare();
            renderScreen();
            ssaoblurShader->setTextureSource("horizontalTex", 1, aoBuffer->colorAttachs[0].texture->id);
            ssaoblurShader->setBool("horizontal", false);
            gBuffer2->prepare(2);
            renderScreen();
        }
    }
    void renderShading(Scene& scene) {
        shadingShader->use();
        scene.setCameraUniforms(*shadingShader);
        scene.setLightUniforms(*shadingShader, ibl != 0);
        shadow_sample.setUniforms(shadingShader, 40);
        shadingShader->setTextureSource("albedoTex", 0, gBuffer1->colorAttachs[0].texture->id);
        shadingShader->setTextureSource("specularTex", 1, gBuffer1->colorAttachs[1].texture->id);
        shadingShader->setTextureSource("emissiveTex", 2, gBuffer1->colorAttachs[2].texture->id);
        shadingShader->setTextureSource("positionTex", 3, gBuffer1->colorAttachs[3].texture->id);
        shadingShader->setTextureSource("normalTex", 4, gBuffer2->colorAttachs[0].texture->id);
        shadingShader->setTextureSource("depthTex", 5, gBuffer2->colorAttachs[1].texture->id);
        shadingShader->setTextureSource("aoTex", 6, gBuffer2->colorAttachs[2].texture->id);
        shadingBuffer->prepare();
        renderScreen();
    }
    void renderReflection(Scene& scene) {
        glViewport(0, 0, width / 8, height / 8);
        ssrtilingShader->use();
        ssr_sample.setUniforms(ssrtilingShader, 8);
        scene.setCameraUniforms(*ssrtilingShader);
        ssrtilingShader->setFloat("threshold", scene.ssr_threshold);
        ssrtilingShader->setTextureSource("albedoTex", 0, gBuffer1->colorAttachs[0].texture->id);
        ssrtilingShader->setTextureSource("specularTex", 1, gBuffer1->colorAttachs[1].texture->id);
        ssrtilingShader->setTextureSource("positionTex", 2, gBuffer1->colorAttachs[3].texture->id);
        ssrtilingShader->setTextureSource("normalTex", 3, gBuffer2->colorAttachs[0].texture->id);
        ssrtilingShader->setTextureSource("lineardepthTex", 4, gBuffer2->colorAttachs[1].texture->id);
        ssrtilingShader->setTextureSource("colorTex", 5, shadingBuffer->colorAttachs[0].texture->id);
        tilingBuffer->prepare();
        renderScreen();

        glViewport(0, 0, width / 2, height / 2);
        ssrShader->use();
        ssr_sample.setUniforms(ssrShader, 8);
        scene.setCameraUniforms(*ssrShader);
        ssrShader->setFloat("threshold", scene.ssr_threshold);
        ssrShader->setInt("width", width);
        ssrShader->setInt("height", height);
        ssrShader->setTextureSource("rayamountTex", 0, tilingBuffer->colorAttachs[0].texture->id);
        ssrShader->setTextureSource("specularTex", 1, gBuffer1->colorAttachs[1].texture->id);
        ssrShader->setTextureSource("positionTex", 2, gBuffer1->colorAttachs[3].texture->id);
        ssrShader->setTextureSource("normalTex", 3, gBuffer2->colorAttachs[0].texture->id);
        ssrShader->setTextureSource("lineardepthTex", 4, gBuffer2->colorAttachs[1].texture->id);
        ssrShader->setTextureSource("depthTex", 5, gBuffer1->depthAttach.texture->id);
        rayTraceBuffer->prepare();
        renderScreen();

        glViewport(0, 0, width / 2, height / 2);
        resolveShader->use();
        scene.setCameraUniforms(*resolveShader);
        resolveShader->setInt("width", width);
        resolveShader->setInt("height", height);
        resolveShader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("albedoTex", 1, gBuffer1->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("specularTex", 2, gBuffer1->colorAttachs[1].texture->id);
        resolveShader->setTextureSource("positionTex", 3, gBuffer1->colorAttachs[3].texture->id);
        resolveShader->setTextureSource("normalTex", 4, gBuffer2->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("hitPt1234", 5, rayTraceBuffer->colorAttachs[0].texture->id);
        resolveShader->setTextureSource("hitPt5678", 6, rayTraceBuffer->colorAttachs[1].texture->id);
        resolveShader->setTextureSource("weight1234", 7, rayTraceBuffer->colorAttachs[2].texture->id);
        resolveShader->setTextureSource("weight5678", 8, rayTraceBuffer->colorAttachs[3].texture->id);
        reflectionBuffer->prepare(0);
        renderScreen();

        ssrblurShader->use();
        ssrblurShader->setTextureSource("colorTex", 0, reflectionBuffer->colorAttachs[0].texture->id);
        ssrblurShader->setTextureSource("specularTex", 2, gBuffer1->colorAttachs[1].texture->id);
        ssrblurShader->setBool("horizontal", true);
        reflectionBuffer->prepare(1);
        renderScreen();
        ssrblurShader->setTextureSource("horizontalTex", 1, reflectionBuffer->colorAttachs[1].texture->id);
        ssrblurShader->setBool("horizontal", false);
        reflectionBuffer->prepare(2);
        renderScreen();

        glViewport(0, 0, width, height);
        joinShader->use();
        joinShader->setBool("join", true);
        joinShader->setBool("substract", false);
        joinShader->setBool("tone_mapping", false);
        joinShader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[0].texture->id);
        joinShader->setTextureSource("joinTex", 1, reflectionBuffer->colorAttachs[2].texture->id);
        joinBuffer->prepare();
        renderScreen();
    }
    void renderEffects() {
        bloomShader->use();
        bloomShader->setVec2("pixel_size", pixel_size);
        bloomShader->setTextureSource("colorTex", 0, shadingBuffer->colorAttachs[1].texture->id);
        bloomShader->setTextureSource("horizontalTex", 1, effectBuffer->colorAttachs[0].texture->id);
        bloomShader->setBool("horizontal", true);
        effectBuffer->prepare(0);
        renderScreen();

        bloomShader->setBool("horizontal", false);
        effectBuffer->prepare(1);
        renderScreen();
    }
    void joinEffects(Texture *colorTex) {
        joinShader->use();
        joinShader->setBool("join", effect != 0);
        joinShader->setBool("substract", effect != 0);
        joinShader->setBool("tone_mapping", true);
        joinShader->setTextureSource("colorTex", 0, colorTex->id);
        joinShader->setTextureSource("joinTex", 1, effectBuffer->colorAttachs[1].texture->id);
        screenBuffer->prepare();
        renderScreen();
    }
    void renderScene(Scene& scene) {
        renderGeometry(scene);
        renderShading(scene);
        if(effect)
            renderEffects();
        if (ssr) {
            renderReflection(scene);
            joinEffects(joinBuffer->colorAttachs[0].texture);
        }
        else {
            joinEffects(shadingBuffer->colorAttachs[0].texture);
        }
    }
};