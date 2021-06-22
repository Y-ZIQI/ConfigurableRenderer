#pragma once
#include "shader.h"
#include "renderpass.h"

#include "tex/SearchTex.h"
#include "tex/AreaTex.h"

class SMAA {
public:
	ScreenPass edgePass;
	ScreenPass blendPass;
	ScreenPass neighborPass;
	FrameBuffer* edgeBuffer;
	FrameBuffer* blendBuffer;
	FrameBuffer* screenBuffer;
	Texture* areaTex;
	Texture* searchTex;
	uint width, height;
	float resolution = 1.0f;

	SMAA(uint Width, uint Height) {
		width = Width; height = Height;
		edgeBuffer = new FrameBuffer;
		edgeBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
		blendBuffer = new FrameBuffer;
		blendBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 0);
		screenBuffer = new FrameBuffer;
		screenBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), 0);

		edgePass.setShader(sManager.getShader(SID_SMAA_EDGEPASS));
		blendPass.setShader(sManager.getShader(SID_SMAA_BLENDPASS));
		neighborPass.setShader(sManager.getShader(SID_SMAA_NEIGHBORPASS));

		areaTex = Texture::createFromArray(AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes, GL_LINEAR);
		searchTex = Texture::createFromArray(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes, GL_NEAREST);
	}
	void setResolution(float resolution) {
		this->resolution = resolution;
	}
	void Draw(Texture* no_aa_image) {
		edgeDetection(no_aa_image);
		blendCalculation();
		neighborBlending(no_aa_image);
	}
	void edgeDetection(Texture* no_aa_image) {
		edgePass.shader->use();
		edgePass.shader->setFloat("resolution", resolution);
		edgePass.shader->setTextureSource("Texture", 0, no_aa_image->id);
		edgeBuffer->clear();
		edgeBuffer->prepare();
		edgePass.render();
	}
	void blendCalculation() {
		blendPass.shader->use();
		blendPass.shader->setFloat("resolution", resolution);
		blendPass.shader->setTextureSource("edgeTex_linear", 0, edgeBuffer->colorAttachs[0].texture->id);
		blendPass.shader->setTextureSource("areaTex", 1, areaTex->id);
		blendPass.shader->setTextureSource("searchTex", 2, searchTex->id);
		blendPass.shader->setFloat("width", width);
		blendPass.shader->setFloat("height", height);
		blendPass.shader->setVec4("subsampleIndices", glm::vec4(0.0, 0.0, 0.0, 0.0));
		blendBuffer->clear();
		blendBuffer->prepare();
		blendPass.render();
	}
	void neighborBlending(Texture* no_aa_image) {
		neighborPass.shader->use();
		neighborPass.shader->setFloat("resolution", resolution);
		neighborPass.shader->setTextureSource("blendTex", 0, blendBuffer->colorAttachs[0].texture->id);
		neighborPass.shader->setTextureSource("screenTex", 1, no_aa_image->id);
		neighborPass.shader->setFloat("width", width);
		neighborPass.shader->setFloat("height", height);
		screenBuffer->prepare();
		neighborPass.render();
	}
};