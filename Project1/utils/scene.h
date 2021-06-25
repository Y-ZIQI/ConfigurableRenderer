#pragma once
#include "light.h"
#include "model.h"
#include "camera.h"
#include "texture.h"

using namespace std;

class Scene {
public:
	Camera* camera;
	vector<Camera*> cameras;
	vector<Model*> models;
	vector<PointLight*> ptLights;
	vector<DirectionalLight*> dirLights;
	vector<glm::mat4> modelMats;
	vector<glm::mat3> normalModelMats;

	vector<Texture*> envMaps;
	uint envIdx;
	bool envMap_enabled = false;

	jsonxx::json jscene;

	Scene() {};
	void addCamera(Camera* newCamera) { cameras.push_back(newCamera); }
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
	void loadScene(string const& path) {
		try
		{
			std::ifstream ifs(path);
			ifs >> jscene;
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SCENE::FILE_NOT_SUCCESFULLY_READ" << std::endl;
			return;
		}
		clearScene();
		uint mid = 0;
		auto models = jscene["models"].as_array();
		for (uint i = 0; i < models.size(); i++) {
			std::string mpath = models[i]["file"].as_string();
			std::string directory = path.substr(0, path.find_last_of('/'));
			mpath = directory + '/' + mpath;
			auto instances = models[i]["instances"].as_array();
			for (uint j = 0; j < instances.size(); j++) {
				loadModel(mpath);
				auto translation = instances[j]["translation"].as_array();
				auto scaling = instances[j]["scaling"].as_array();
				auto rotation = instances[j]["rotation"].as_array();
				glm::mat4 modelmat = glm::mat4(1.0);
				modelmat = glm::translate(modelmat, glm::vec3(translation[0].as_float(), translation[1].as_float(), translation[2].as_float()));
				modelmat = glm::scale(modelmat,glm::vec3(scaling[0].as_float(), scaling[1].as_float(), scaling[2].as_float()));
				//TODO: Rotate
				//modelmat = glm::rotate(modelmat, rotation[0], glm::vec3(1.0, 0.0, 0.0));
				setModelMat(mid++, modelmat);
			}
		}

		auto lights = jscene["lights"].as_array();
		for (uint i = 0; i < lights.size(); i++) {
			std::string name = lights[i]["name"].as_string();
			std::string type = lights[i]["type"].as_string();
			auto intensity = lights[i]["intensity"].as_array();
			auto direction = lights[i]["direction"].as_array();
			auto pos = lights[i]["pos"].as_array();
			float ambient = lights[i]["ambient"].as_float();
			if (type == "dir_light") {
				addLight(new DirectionalLight(
					name,
					glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
					glm::vec3(direction[0].as_float(), direction[1].as_float(), direction[2].as_float()),
					glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
					ambient));
			}
			else if (type == "point_light") {
				float range = lights[i]["range"].as_float();
				addLight(new PointLight(
					name,
					glm::vec3(intensity[0].as_float(), intensity[1].as_float(), intensity[2].as_float()),
					glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
					glm::vec3(direction[0].as_float(), direction[1].as_float(), direction[2].as_float()),
					ambient, range));
			}
		}

		auto cameras = jscene["cameras"].as_array();
		for (uint i = 0; i < cameras.size(); i++) {
			//std::string name = cameras[i]["name"].as_string();
			auto pos = cameras[i]["pos"].as_array();
			auto target = cameras[i]["target"].as_array();
			auto up = cameras[i]["up"].as_array();
			auto depth_range = cameras[i]["depth_range"].as_array();
			float focal_length = cameras[i]["focal_length"].as_float();
			float aspect_ratio = cameras[i]["aspect_ratio"].as_float();
			Camera* nCamera = new Camera(
				glm::vec3(pos[0].as_float(), pos[1].as_float(), pos[2].as_float()),
				glm::vec3(target[0].as_float(), target[1].as_float(), target[2].as_float()),
				glm::vec3(up[0].as_float(), up[1].as_float(), up[2].as_float()),
				focal_length, aspect_ratio, depth_range[0].as_float(), depth_range[1].as_float()
			);
			addCamera(nCamera);
		}
		setCamera(this->cameras[0]);
	}
	void clearScene() {
		//TODO: clear loaded data
	};
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
				glViewport(0, 0, sm_ptr->width, sm_ptr->height);
				sm_ptr->smBuffer->clear();
				sm_ptr->smBuffer->prepare();
				dirLights[i]->smShader->use();
				dirLights[i]->smShader->setMat4("viewProj", dirLights[i]->viewProj);
				DrawMesh(*dirLights[i]->smShader);
				sm_ptr->smBuffer->colorAttachs[0].texture->genMipmap();
			}
		}
		for (int i = 0; i < ptLights.size(); i++) {
			if (ptLights[i]->shadow_enabled) {
				sm_ptr = ptLights[i]->shadowMap;
				glEnable(GL_DEPTH_TEST);
				glViewport(0, 0, sm_ptr->width, sm_ptr->height);
				sm_ptr->smBuffer->clear();
				sm_ptr->smBuffer->prepare();
				ptLights[i]->smShader->use();
				ptLights[i]->smShader->setMat4("viewProj", ptLights[i]->viewProj);
				DrawMesh(*ptLights[i]->smShader);
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
				shader.setMat4(tmp, dirLights[i]->viewProj);
				sprintf(tmp, "dirLights[%d].shadowMap", i);
				shader.setTextureSource(tmp, sm_index, dirLights[i]->shadowMap->smBuffer->colorAttachs[0].texture->id);
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
				shader.setMat4(tmp, ptLights[i]->viewProj);
				sprintf(tmp, "ptLights[%d].shadowMap", i);
				shader.setTextureSource(tmp, sm_index, ptLights[i]->shadowMap->smBuffer->colorAttachs[0].texture->id);
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