#pragma once
#include "light.h"
#include "model.h"
#include "camera.h"
#include "texture.h"

using namespace std;

class Scene {
public:
	Camera* camera;
	vector<Model*> models;
	vector<PointLight*> ptLights;
	vector<DirectionalLight*> dirLights;
	vector<glm::mat4> modelMats;
	vector<glm::mat3> normalModelMats;

	vector<Texture*> envMaps;
	uint envIdx;
	bool envMap_enabled = false;

	Scene() {};
	void setCamera(Camera& newCamera) { camera = &newCamera; };
	void setCamera(Camera* newCamera) { camera = newCamera; };
	void addLight(PointLight* nLight) { ptLights.push_back(nLight); };
	void addLight(DirectionalLight* nLight) { dirLights.push_back(nLight); };
	void addModel(Model* nModel) {
		models.push_back(nModel);
		modelMats.push_back(glm::mat4(1.0));
		normalModelMats.push_back(glm::mat4(1.0));
	};
	void loadModel(string const& path, bool gamma = false) {
		Model* m = new Model(path, gamma);
		addModel(m);
	}
	void setModelMat(uint idx, glm::mat4 &nMat) {
		if (idx >= modelMats.size() || idx >= normalModelMats.size())
			return;
		modelMats[idx] = nMat;
		normalModelMats[idx] = glm::transpose(glm::inverse(glm::mat3(nMat)));
	}
	void addEnvMap(Texture* envMap) {
		if (!envMap_enabled) envMap_enabled = true;
		envMaps.push_back(envMap); 
	};
	void setEnvMap(uint idx) { envIdx = idx; };
	void Draw(Shader& shader, uint blend_mode = 0, bool draw_envmap = true, bool deferred = true) {
		if (blend_mode == 0) {
			for (uint i = 0; i < models.size(); i++) {
				setModelUniforms(shader, i);
				models[i]->Draw(shader);
			}
			if (draw_envmap && envMap_enabled && envIdx >= 0)
				DrawEnvMap(*sManager.getShader(deferred ? SID_DEFERRED_ENVMAP : SID_FORWARD_ENVMAP));
		}else{
			for (uint i = 0; i < models.size(); i++) {
				setModelUniforms(shader, i);
				models[i]->DrawOpaque(shader);
			}
			if(draw_envmap && envMap_enabled && envIdx >= 0)
				DrawEnvMap(*sManager.getShader(deferred ? SID_DEFERRED_ENVMAP : SID_FORWARD_ENVMAP));
			glm::mat4 viewProj = camera->GetProjMatrix() * camera->GetViewMatrix();
			shader.use();
			for (uint i = 0; i < models.size(); i++) {
				models[i]->OrderTransparent(viewProj * modelMats[i]);
				models[i]->DrawTransparent(shader);
			}
		}
	}
	void DrawMesh(Shader& shader) {
		for (uint i = 0; i < models.size(); i++) {
			setModelUniforms(shader, i, false);
			models[i]->DrawMesh(shader);
		}
	}
	void DrawEnvMap(Shader& shader) {
		if(!is_envmap_vao_initialized){
			glGenVertexArrays(1, &_envmap_vao);
			glBindVertexArray(_envmap_vao);
			glGenBuffers(1, &_envmap_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, _envmap_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(_envMapVertices), &_envMapVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glBindVertexArray(0);
			is_envmap_vao_initialized = true;
		}
		shader.use();
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		setCameraUniforms(shader, true);
		shader.setTextureSource("envmap", 0, envMaps[envIdx]->id, GL_TEXTURE_CUBE_MAP);
		glBindVertexArray(_envmap_vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
	}
	void update() {
		camera->update();
		for (int i = 0; i < dirLights.size(); i++)
			dirLights[i]->update();
		for (int i = 0; i < ptLights.size(); i++)
			ptLights[i]->update();
		genShadow();
	}
	void genShadow(bool cull_front = false) {
		if(cull_front)
			glCullFace(GL_FRONT);
		ShadowMap* sm_ptr;
		for (int i = 0; i < dirLights.size(); i++) {
			if (dirLights[i]->shadow_enabled) {
				sm_ptr = dirLights[i]->shadowMap;
				glEnable(GL_DEPTH_TEST);
				glViewport(0, 0, sm_ptr->width * sm_ptr->resolution, sm_ptr->height * sm_ptr->resolution);
				sm_ptr->smBuffer->clear();
				sm_ptr->smBuffer->prepare();
				sm_ptr->shader->use();
				sm_ptr->shader->setMat4("viewProj", sm_ptr->viewProj);
				DrawMesh(*sm_ptr->shader);
				sm_ptr->smBuffer->colorAttachs[0].texture->genMipmap();
			}
		}
		for (int i = 0; i < ptLights.size(); i++) {
			if (ptLights[i]->shadow_enabled) {
				sm_ptr = ptLights[i]->shadowMap;
				glEnable(GL_DEPTH_TEST);
				glViewport(0, 0, sm_ptr->width * sm_ptr->resolution, sm_ptr->height * sm_ptr->resolution);
				sm_ptr->smBuffer->clear();
				sm_ptr->smBuffer->prepare();
				sm_ptr->shader->use();
				sm_ptr->shader->setMat4("viewProj", sm_ptr->viewProj);
				DrawMesh(*sm_ptr->shader);
				sm_ptr->smBuffer->colorAttachs[0].texture->genMipmap();
			}
		}
		if (cull_front)
			glCullFace(GL_BACK);
	}
	void setModelUniforms(Shader& shader, uint model_index, bool normal_model = true) {
		if (normal_model)
			shader.setMat3("normal_model", normalModelMats[model_index]);
		shader.setMat4("model", modelMats[model_index]);
	}
	void setVPUniforms(Shader& shader, bool remove_trans = false) {
		// view/projection transformations
		glm::mat4 projectionMat = camera->GetProjMatrix();
		glm::mat4 viewMat = camera->GetViewMatrix();
		if(remove_trans)
			viewMat = glm::mat4(glm::mat3(viewMat));
		shader.setMat4("projection", projectionMat);
		shader.setMat4("view", viewMat);
	}
	void setLightUniforms(Shader& shader) {
		shader.setInt("dirLtCount", dirLights.size());
		shader.setInt("ptLtCount", ptLights.size());
		char tmp[64];
		int sm_index = 5;
		for (int i = 0; i < dirLights.size(); i++) {
			sprintf(tmp, "dirLights[%d].intensity", i);
			shader.setVec3(tmp, dirLights[i]->intensity);
			sprintf(tmp, "dirLights[%d].direction", i);
			shader.setVec3(tmp, dirLights[i]->direction);
			sprintf(tmp, "dirLights[%d].ambient", i);
			shader.setFloat(tmp, dirLights[i]->ambient);
			sprintf(tmp, "dirLights[%d].has_shadow", i);
			shader.setBool(tmp, dirLights[i]->shadow_enabled);
			if (dirLights[i]->shadow_enabled) {
				sprintf(tmp, "dirLights[%d].viewProj", i);
				shader.setMat4(tmp, dirLights[i]->shadowMap->viewProj);
				sprintf(tmp, "dirLights[%d].shadowMap", i);
				//shader.setTextureSource(tmp, sm_index, dirLights[i]->shadowMap->smBuffer->depthAttach.texture->id);
				shader.setTextureSource(tmp, sm_index, dirLights[i]->shadowMap->smBuffer->colorAttachs[0].texture->id);
				sprintf(tmp, "dirLights[%d].resolution", i);
				shader.setFloat(tmp, dirLights[i]->shadowMap->resolution);
				sm_index++;
			}
		}
		for (int i = 0; i < ptLights.size(); i++) {
			sprintf(tmp, "ptLights[%d].intensity", i);
			shader.setVec3(tmp, ptLights[i]->intensity);
			sprintf(tmp, "ptLights[%d].position", i);
			shader.setVec3(tmp, ptLights[i]->position);
			sprintf(tmp, "ptLights[%d].ambient", i);
			shader.setFloat(tmp, ptLights[i]->ambient);
			sprintf(tmp, "ptLights[%d].constant", i);
			shader.setFloat(tmp, ptLights[i]->constant);
			sprintf(tmp, "ptLights[%d].linear", i);
			shader.setFloat(tmp, ptLights[i]->linear);
			sprintf(tmp, "ptLights[%d].quadratic", i);
			shader.setFloat(tmp, ptLights[i]->quadratic);
			sprintf(tmp, "ptLights[%d].has_shadow", i);
			shader.setBool(tmp, ptLights[i]->shadow_enabled);
			if (ptLights[i]->shadow_enabled) {
				sprintf(tmp, "ptLights[%d].viewProj", i);
				shader.setMat4(tmp, ptLights[i]->shadowMap->viewProj);
				sprintf(tmp, "ptLights[%d].shadowMap", i);
				//shader.setTextureSource(tmp, sm_index, ptLights[i]->shadowMap->smBuffer->depthAttach.texture->id);
				shader.setTextureSource(tmp, sm_index, ptLights[i]->shadowMap->smBuffer->colorAttachs[0].texture->id);
				sprintf(tmp, "ptLights[%d].resolution", i);
				shader.setFloat(tmp, ptLights[i]->shadowMap->resolution);
				sm_index++;
			}
		}
	}
	void setCameraUniforms(Shader& shader, bool remove_trans = false) {
		float inv_n = 1.0f / camera->nearZ, inv_f = 1.0f / camera->farZ;
		glm::mat4 projectionMat = camera->GetProjMatrix();
		glm::mat4 viewMat = camera->GetViewMatrix();
		if (remove_trans)
			viewMat = glm::mat4(glm::mat3(viewMat));
		shader.setVec3("camera_pos", camera->Position);
		shader.setVec4("camera_params", glm::vec4(camera->Zoom, camera->Aspect, inv_n + inv_f, inv_n - inv_f));
		shader.setMat4("camera_vp", projectionMat * viewMat);
	}
	void setShaderData(Shader& shader) {
		setLightUniforms(shader);
		setCameraUniforms(shader);
	}
};