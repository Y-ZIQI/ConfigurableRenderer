#pragma once

#include "framebuffer.h"
#include "shader.h"

class ShadowMap {
public:
	bool initialized = false;
	FrameBuffer* smBuffer;
	uint width, height;

	ShadowMap(uint Width, uint Height) {
		initialized = true;
		width = Width; height = Height;
		smBuffer = new FrameBuffer;
		smBuffer->addDepthStencil(width, height);
		Texture* ntex = Texture::create(width, height, GL_R32F, GL_RED, GL_FLOAT);
		ntex->setTexParami(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		ntex->setTexParami(GL_TEXTURE_BASE_LEVEL, 0);
		ntex->setTexParami(GL_TEXTURE_MAX_LEVEL, 3);
		smBuffer->attachColorTarget(ntex, 0);
		//ntex->setTexParami(GL_TEXTURE_REDUCTION_MODE_ARB, GL_MAX);
	};
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
	std::vector<ShadowMap*> smList;
	ShadowMap* shadowMap;
	Shader* smShader;
	glm::mat4 viewProj, viewMat, projMat;

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
			viewMat = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
			projMat = glm::ortho(-rangeX / 2.0f, rangeX / 2.0f, -rangeY / 2.0f, rangeY / 2.0f, 0.0f, rangeZ);
			viewProj = projMat * viewMat;
		}
	}
	void enableShadow(float rangeX, float rangeY, float rangeZ, std::vector<uint> width_list) {
		if (!shadow_enabled) {
			shadow_enabled = true;
			smList.resize(width_list.size());
			for(uint i = 0;i < width_list.size();i++)
				smList[i] = new ShadowMap(width_list[i], width_list[i]);
			shadowMap = smList[0];
			smShader = sManager.getShader(SID_SHADOWMAP);
		}
		this->rangeX = rangeX;
		this->rangeY = rangeY;
		this->rangeZ = rangeZ;
		update();
	}
	void setShadowMap(uint idx) {
		if (shadow_enabled)
			shadowMap = smList[idx];
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
			viewMat = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
			projMat = glm::perspective(glm::radians(Zoom), Aspect, nearZ, farZ);
			viewProj = projMat * viewMat;
		}
	}
	void enableShadow(float Zoom, float nearZ, float farZ, std::vector<uint> width_list) {
		if (!shadow_enabled) {
			shadow_enabled = true;
			smList.resize(width_list.size());
			for (uint i = 0; i < width_list.size(); i++)
				smList[i] = new ShadowMap(width_list[i], width_list[i]);
			shadowMap = smList[0];
			smShader = sManager.getShader(SID_SHADOWMAP);
		}
		this->nearZ = nearZ;
		this->farZ = farZ;
		this->Zoom = Zoom;
		this->Aspect = 1.0f;
		update();
	}
	void setShadowMap(uint idx) {
		if (shadow_enabled)
			shadowMap = smList[idx];
	}
};