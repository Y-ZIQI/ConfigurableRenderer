#pragma once
#include "shader.h"
#include "scene.h"
#include "camera.h"
#include "framebuffer.h"

class RenderPass {
public:
	Shader* shader;

	RenderPass() {};
	void setShader(Shader* Shader) {
		this->shader = Shader;
	};
	void reload() {
		shader->reload();
	}
};

class RasterScenePass : public RenderPass {
public:
	void renderScene(Scene& scene, uint blend_mode = 0, bool draw_envmap = true, bool defered = true) {
		shader->use();
		glEnable(GL_DEPTH_TEST);
		scene.Draw(*shader, blend_mode, draw_envmap, defered);
	}
};

class ScreenPass : public RenderPass {
public:
	ScreenPass() {
		if (!is_screen_vao_initialized) {
			glGenVertexArrays(1, &_screen_vao);
			glBindVertexArray(_screen_vao);
			glGenBuffers(1, &_screen_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, _screen_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			glBindVertexArray(0);
			is_screen_vao_initialized = true;
		}
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
		scene.Draw(*shader, 0, false, false);
	}
	void setBaseUniforms(Scene& scene) {
		shader->setMat4("transforms[0]", transforms[0]);
		shader->setMat4("transforms[1]", transforms[1]);
		shader->setMat4("transforms[2]", transforms[2]);
		shader->setMat4("transforms[3]", transforms[3]);
		shader->setMat4("transforms[4]", transforms[4]);
		shader->setMat4("transforms[5]", transforms[5]);
	}
};

class ForwardRenderer {
public:
	RasterScenePass basePass;

	ForwardRenderer() {
		Shader* shader = sManager.getShader(SID_FORWARD);
		basePass.setShader(shader);
	}
	void reload() {
		basePass.reload();
	}
	void renderScene(Scene& scene, FrameBuffer& fbo) {
		basePass.shader->use();
		setShaderData(scene);
		fbo.prepare();
		basePass.renderScene(scene, 0, true, false);
	}
	void setShaderData(Scene& scene) {
		scene.setShaderData(*basePass.shader);
	}
};

class DeferredRenderer {
public:
	RasterScenePass basePass;
	ScreenPass ssaoPass;
	ScreenPass shadingPass;
	FrameBuffer* gBuffer;
	FrameBuffer* aoBuffer;
	uint width, height;

	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;

	uint ssao = 0;

	DeferredRenderer(uint Width, uint Height) {
		width = Width; height = Height;
		gBuffer = new FrameBuffer;
		gBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT), 0);
		gBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT), 1);
		gBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), 2);
		gBuffer->attachColorTarget(Texture::create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), 3);
		//gBuffer->attachDepthTarget(Texture::create(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
		gBuffer->addDepthStencil(width, height);
		bool is_success = gBuffer->checkStatus();
		if (!is_success) {
			cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
			return;
		}
		aoBuffer = new FrameBuffer;
		aoBuffer->attachColorTarget(Texture::create(width, height, GL_R16F, GL_RED, GL_FLOAT), 0);

		basePass.setShader(sManager.getShader(SID_DEFERRED_BASE));
		ssaoPass.setShader(sManager.getShader(SID_SSAO));
		shadingPass.setShader(sManager.getShader(SID_DEFERRED_SHADING));

		std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
		char tmp[64];
		for (int i = 0; i < 64; i++) {
			glm::vec3 sample(
				randomFloats(generator) * 2.0 - 1.0,
				randomFloats(generator) * 2.0 - 1.0,
				randomFloats(generator)
			);
			sample = glm::normalize(sample);
			sample *= randomFloats(generator);
			ssaoKernel.push_back(sample);
		}
	}
	void reload() {
		basePass.reload();
		shadingPass.reload();
	}
	void renderGeometry(Scene& scene, float resolution = 1.0f) {
		glViewport(0, 0, width * resolution, height * resolution);
		basePass.shader->use();
		gBuffer->clear();
		gBuffer->prepare();
		setBaseUniforms(scene);
		basePass.renderScene(scene, 0, true, true);
	}
	void renderAO(Scene& scene, float resolution = 1.0f) {
		ssaoPass.shader->use();
		aoBuffer->clear();
		aoBuffer->prepare();
		setSSAOUniforms(scene, 16, resolution);
		ssaoPass.render();
	}
	void renderScreen(Scene& scene, FrameBuffer& fbo, float resolution = 1.0f) {
		shadingPass.shader->use();
		fbo.prepare();
		setShadingUniforms(scene, resolution);
		shadingPass.render();
	}
	void renderScene(Scene& scene, FrameBuffer& fbo, float resolution = 1.0f) {
		renderGeometry(scene, resolution);
		if(ssao)
			renderAO(scene, resolution);
		renderScreen(scene, fbo, resolution);
	}
	void setBaseUniforms(Scene& scene) {
		scene.setCameraUniforms(*basePass.shader);
	}
	void setSSAOUniforms(Scene& scene, int sample_num, float resolution = 1.0f) {
		scene.setCameraUniforms(*ssaoPass.shader);
		ssaoPass.shader->setFloat("resolution", resolution);
		ssaoPass.shader->setTextureSource("positionTex", 0, gBuffer->colorAttachs[0].texture->id);
		ssaoPass.shader->setTextureSource("normalTex", 1, gBuffer->colorAttachs[1].texture->id);
		
		char tmp[64];
		for (int i = 0; i < sample_num; i++) {
			sprintf(tmp, "samples[%d]", i);
			ssaoPass.shader->setVec3(tmp, ssaoKernel[i]);
		}
	}
	void setShadingUniforms(Scene& scene, float resolution = 1.0f) {
		scene.setLightUniforms(*shadingPass.shader);
		scene.setCameraUniforms(*shadingPass.shader);
		shadingPass.shader->setFloat("resolution", resolution);
		shadingPass.shader->setTextureSource("positionTex", 0, gBuffer->colorAttachs[0].texture->id);
		shadingPass.shader->setTextureSource("normalTex", 1, gBuffer->colorAttachs[1].texture->id);
		shadingPass.shader->setTextureSource("albedoTex", 2, gBuffer->colorAttachs[2].texture->id);
		shadingPass.shader->setTextureSource("specularTex", 3, gBuffer->colorAttachs[3].texture->id);
		if(ssao)
			shadingPass.shader->setTextureSource("aoTex", 4, aoBuffer->colorAttachs[0].texture->id);
	}
};

class OmnidirectionalRenderer {
public:
	RasterScenePass basePass;
	FrameBuffer* cube;
	uint width, height;
	glm::vec3 position;
	glm::mat4 projMat;
	std::vector<glm::mat4> transforms;

	OmnidirectionalRenderer(uint Width, uint Height) {
		width = Width; height = Height;
		cube = new FrameBuffer;
		cube->attachCubeTarget(Texture::createCubeMap(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), 0);
		cube->attachCubeDepthTarget(Texture::createCubeMap(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
		bool is_success = cube->checkStatus();
		if (!is_success) {
			cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
			return;
		}
		transforms.resize(6);

		Shader* baseShader = sManager.getShader(SID_CUBEMAP_RENDER);
		basePass.setShader(baseShader);
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
		basePass.shader->use();
		cube->clear();
		cube->prepare();
		setBaseUniforms(scene);
		basePass.renderScene(scene, 0, false, false);
	}
	void setBaseUniforms(Scene& scene) {
		basePass.shader->setMat4("transforms[0]", transforms[0]);
		basePass.shader->setMat4("transforms[1]", transforms[1]);
		basePass.shader->setMat4("transforms[2]", transforms[2]);
		basePass.shader->setMat4("transforms[3]", transforms[3]);
		basePass.shader->setMat4("transforms[4]", transforms[4]);
		basePass.shader->setMat4("transforms[5]", transforms[5]);
	}
};