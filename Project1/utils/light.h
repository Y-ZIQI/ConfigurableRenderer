#pragma once

#include "framebuffer.h"
#include "shader.h"

class ShadowMap {
public:
	bool initialized = false;
	FrameBuffer* smBuffer;
	Shader* shader;
	uint width, height;
	float resolution;
	glm::mat4 viewProj, viewMat, projMat;

	ShadowMap() {};
	void initialize(uint Width, uint Height) {
		if (!initialized) {
			initialized = true;
			width = Width; height = Height;
			smBuffer = new FrameBuffer;
			//smBuffer->attachDepthTarget(Texture::create(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
			smBuffer->addDepthStencil(width, height);
			Texture* ntex = Texture::create(width, height, GL_R32F, GL_RED, GL_FLOAT);
			//ntex->setTexParami(GL_TEXTURE_REDUCTION_MODE_ARB, GL_MAX);
			ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			ntex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
			ntex->setTexParami(GL_TEXTURE_MAX_LEVEL, 3);
			smBuffer->attachColorTarget(ntex, 0);
			resolution = 1.0f;
			shader = sManager.getShader(SID_SHADOWMAP);
		}
	}
	void setViewProj(const glm::mat4& vpMat) {
		viewProj = vpMat;
	}
	void setResolution(const float Resolution) {
		resolution = Resolution;
	}
};

enum class LType : int {
	Directional = 1,
	Point = 2,
};

class Light {
public:
	std::string name;
	LType type;

	float ambient;
	glm::vec3 intensity;
	glm::vec3 position;
	glm::vec3 direction;

	bool shadow_enabled = false;
	ShadowMap* shadowMap;

	void addGui(nanogui::FormHelper* gui) {
		gui->addGroup(name);
		gui->addVariable("ambient", ambient)->setSpinnable(true);
		gui->addVariable("Intensity.r", intensity[0])->setSpinnable(true);
		gui->addVariable("Intensity.g", intensity[1])->setSpinnable(true);
		gui->addVariable("Intensity.b", intensity[2])->setSpinnable(true);
		gui->addVariable("Position.x", position[0])->setSpinnable(true);
		gui->addVariable("Position.y", position[1])->setSpinnable(true);
		gui->addVariable("Position.z", position[2])->setSpinnable(true);
		gui->addVariable("Direction.x", direction[0])->setSpinnable(true);
		gui->addVariable("Direction.y", direction[1])->setSpinnable(true);
		gui->addVariable("Direction.z", direction[2])->setSpinnable(true);;
	}
};

class DirectionalLight : public Light {
public:
	// For shadow map
	float rangeX, rangeY, rangeZ;

	DirectionalLight(const std::string& Name, glm::vec3 Intensity, glm::vec3 Direction, glm::vec3 Position = glm::vec3(0.0, 0.0, 0.0), float Ambient = 0.01f) {
		type = LType::Directional;
		name = Name;
		ambient = Ambient;
		intensity = Intensity;
		direction = Direction;
		position = Position;
	}
	void update() {
		if (shadow_enabled) {
			glm::mat4 viewMat = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 projMat = glm::ortho(-rangeX / 2.0f, rangeX / 2.0f, -rangeY / 2.0f, rangeY / 2.0f, 0.0f, rangeZ);
			shadowMap->setViewProj(projMat * viewMat);
		}
	}
	void enableShadow(float rangeX, float rangeY, float rangeZ, uint width = 512, uint height = 512) {
		if (!shadow_enabled) {
			shadow_enabled = true;
			shadowMap = new ShadowMap;
			shadowMap->initialize(width, height);
		}
		this->rangeX = rangeX;
		this->rangeY = rangeY;
		this->rangeZ = rangeZ;
		update();
	}
	void setResolution(const float Resolution) {
		if (shadow_enabled) {
			shadowMap->setResolution(Resolution);
		}
	}
};

class PointLight : public Light {
public:
	// For light fading
	float range, constant, linear, quadratic;
	// For shadow map
	float nearZ, farZ, Zoom, Aspect;

	PointLight(const std::string& Name, glm::vec3 Intensity, glm::vec3 Position, glm::vec3 Direction = glm::vec3(0.0, 0.0, 0.0), float Ambient = 0.01f, float Range = 10.0f) {
		type = LType::Point;
		name = Name;
		ambient = Ambient;
		intensity = Intensity;
		position = Position;
		direction = Direction;
		range = Range;
		constant = 1.0f;
		linear = 5.0f / Range;
		quadratic = 80.0f / (Range * Range);
	}
	/*// Old version
	PointLight(const std::string& Name, glm::vec3 Intensity, glm::vec3 Position, glm::vec3 Direction = glm::vec3(0.0, 0.0, 0.0), float Ambient = 0.01f, float Constant = 1.0f, float Linear = 0.7f, float Quadratic = 1.8f) {
		type = LType::Point;
		name = Name;
		ambient = Ambient;
		intensity = Intensity;
		position = Position;
		direction = Direction;
		constant = Constant;
		linear = Linear;
		quadratic = Quadratic;
	}*/
	void update() {
		if (shadow_enabled) {
			glm::mat4 viewMat = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 projMat = glm::perspective(glm::radians(Zoom), Aspect, nearZ, farZ);
			shadowMap->setViewProj(projMat * viewMat);
		}
	}
	void enableShadow(float Zoom, float nearZ, float farZ, uint width = 512, uint height = 512) {
		if (!shadow_enabled) {
			shadow_enabled = true;
			shadowMap = new ShadowMap;
			shadowMap->initialize(width, height);
		}
		this->nearZ = nearZ;
		this->farZ = farZ;
		this->Zoom = Zoom;
		this->Aspect = (float)width / height;
		update();
	}
	void setResolution(const float Resolution) {
		if (shadow_enabled) {
			shadowMap->setResolution(Resolution);
		}
	}
};