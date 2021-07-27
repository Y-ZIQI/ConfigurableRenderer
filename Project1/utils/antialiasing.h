#pragma once
#include "shader.h"
#include "renderpass.h"

class SMAA {
public:
    Shader* edgeShader;
    Shader* blendShader;
    Shader* neighborShader;
    FrameBuffer* edgeBuffer;
    FrameBuffer* blendBuffer;
    FrameBuffer* screenBuffer;
    Texture* areaTex;
    Texture* searchTex;
    uint width, height;

    SMAA(uint Width, uint Height) {
        width = Width; height = Height;
        edgeBuffer = new FrameBuffer;
        edgeBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
        blendBuffer = new FrameBuffer;
        blendBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 0);
        screenBuffer = new FrameBuffer;
        screenBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 0);

        edgeShader = sManager.getShader(SID_SMAA_EDGEPASS);
        blendShader = sManager.getShader(SID_SMAA_BLENDPASS);
        neighborShader = sManager.getShader(SID_SMAA_NEIGHBORPASS);

        areaTex = tManager.getTex(TID_SMAA_AREATEX);
        searchTex = tManager.getTex(TID_SMAA_SEARCHTEX);
    }
    void Draw(Texture* no_aa_image) {
        edgeDetection(no_aa_image);
        blendCalculation();
        neighborBlending(no_aa_image);
    }
    void edgeDetection(Texture* no_aa_image) {
        edgeShader->use();
        edgeShader->setTextureSource("Texture", 0, no_aa_image->id);
        edgeBuffer->prepare();
        edgeBuffer->clear();
        renderScreen();
    }
    void blendCalculation() {
        blendShader->use();
        blendShader->setTextureSource("edgeTex_linear", 0, edgeBuffer->colorAttachs[0].texture->id);
        blendShader->setTextureSource("areaTex", 1, areaTex->id);
        blendShader->setTextureSource("searchTex", 2, searchTex->id);
        blendShader->setFloat("width", width);
        blendShader->setFloat("height", height);
        blendShader->setVec4("subsampleIndices", glm::vec4(0.0, 0.0, 0.0, 0.0));
        blendBuffer->prepare();
        blendBuffer->clear();
        renderScreen();
    }
    void neighborBlending(Texture* no_aa_image) {
        neighborShader->use();
        neighborShader->setTextureSource("blendTex", 0, blendBuffer->colorAttachs[0].texture->id);
        neighborShader->setTextureSource("screenTex", 1, no_aa_image->id);
        neighborShader->setFloat("width", width);
        neighborShader->setFloat("height", height);
        screenBuffer->prepare();
        renderScreen();
    }
};