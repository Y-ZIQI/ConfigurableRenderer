#pragma once
#include "AllInclude.h"

const float values[][3] = {
	{0.6f, 0.8f, 1.0f},
	{0.25f, 0.5f, 1.0f}
};
const std::vector<uint> sm_width_list{ 512, 1024, 2048 };
enum Level : uint {
	level1 = 0,
	level2,
	level3
};
enum Mode : uint {
	mode1 = 0,
	mode2
};
class Settings {
public:
	/** Resolution ratio
	* 1: 60%
	* 2: 80%
	* 3: 100%
	*/
	Level resolution;
	/** Shadow Resolution ratio
	* 1: 25%
	* 2: 50%
	* 3: 100%
	*/
	Level shadow_resolution;
	/** SSAO level
	* 1: no SSAO
	* 2: SSAO sample 4
	* 3: SSAO sample 16
	*/
	Level ssao_level;
	/** Shading
	* 1: TODO: 
	* 2: TODO: 
	* 3: TODO: 
	*/
	Level shading;
	/** SMAA level
	* 1: no SMAA
	* 2: has SMAA
	* 3: TODO
	*/
	Level smaa_level;

	Settings() {
		resolution = level3;
		shadow_resolution = level3;
		ssao_level = level1;
		shading = level1;
		smaa_level = level1;
	}
};

class RenderFrame {
public:
	RenderFrame(int width, int height, const char* title);
	void onLoad();
	void onFrameRender();
	void processInput();
	void config(bool force_update);
	void run();

	Settings settings, stHistory;

private:
	int width, height;
	float resolution = 1.0f;

	GLFWENV* glfw;
	Scene* scene;
	Camera* camera;

	ScreenPass* resolvePass;
	std::vector<DeferredRenderer*> dRdrList;
	std::vector<FrameBuffer*> sFboList;
	std::vector<SMAA*> smaaList;
	FrameBuffer* targetFbo;
	DeferredRenderer* dRenderer;
	FrameBuffer* screenFbo;
	SMAA* smaaPass;

	OmnidirectionalPass* oPass;
	
	nanogui::Screen* screen = nullptr;
	nanogui::FormHelper* gui;
	nanogui::ref<nanogui::Window> nanoguiWindow;

	bool enabled = true;
	bool moving = false;

	/**
	* 1: Bistro
	* 2: SunTemple
	*/
	uint test_scene = 2;
	float fps, duration;
	Record record[2]; // All, SMAA+resolve
	float time_ratio[5]; // Shadow, Draw, AO, Shading, Post
};

RenderFrame::RenderFrame(int width, int height, const char* title) {
	this->width = width;
	this->height = height;
	glfw = new GLFWENV(width, height, title, true);
	
	screen = new nanogui::Screen();
	screen->initialize(glfw->window, true);
	
	gui = new nanogui::FormHelper(screen);
	nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Demo");
	gui->addGroup("Status");
	gui->addVariable("FPS", fps, false);
	gui->addVariable("Shadow", time_ratio[0], false);
	gui->addVariable("Draw", time_ratio[1], false);
	gui->addVariable("Shading", time_ratio[2], false);
	gui->addVariable("AO", time_ratio[3], false);
	gui->addVariable("Post", time_ratio[4], false);
	gui->addGroup("Config");
	gui->addVariable("Resolution", settings.resolution, enabled)->setItems({ "60%", "80%", "100%" });
	gui->addVariable("Shadow Resolution", settings.shadow_resolution, enabled)->setItems({ "512", "1024", "2048" });
	gui->addVariable("Shading", settings.shading, enabled)->setItems({ "1", "2", "3" });
	gui->addVariable("SSAO Level", settings.ssao_level, enabled)->setItems({ "Disable", "1", "2" });
	gui->addVariable("SMAA Level", settings.smaa_level, enabled)->setItems({ "Disable", "1", "2" });
}

void RenderFrame::config(bool force_update = false) {
	if (settings.resolution != stHistory.resolution || force_update) {
		stHistory.resolution = settings.resolution;
		resolution = values[0][settings.resolution];
		dRenderer = dRdrList[settings.resolution];
		screenFbo = sFboList[settings.resolution];
		smaaPass = smaaList[settings.resolution];
	}
	if (settings.shadow_resolution != stHistory.shadow_resolution || force_update) {
		stHistory.shadow_resolution = settings.shadow_resolution;
		for (int i = 0; i < scene->dirLights.size(); i++)
			scene->dirLights[i]->setShadowMap(settings.shadow_resolution);
		for (int i = 0; i < scene->ptLights.size(); i++)
			scene->ptLights[i]->setShadowMap(settings.shadow_resolution);
	}
	if (settings.shading != stHistory.shading || force_update) {
		stHistory.shading = settings.shading;
		if (settings.shading == level1) {
			//sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "EVAL_SPECULAR");
			//sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "EVAL_SPECULAR");
			sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SHADING_MODEL", "1");
		}
		else if (settings.shading == level2) {
			//sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "EVAL_SPECULAR");
			//sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "EVAL_SPECULAR");
			sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SHADING_MODEL", "2");
		}
		else if (settings.shading == level3) {
		}
		sManager.getShader(SID_DEFERRED_SHADING)->reload();
	}
	if (settings.ssao_level != stHistory.ssao_level || force_update) {
		stHistory.ssao_level = settings.ssao_level;
		dRdrList[0]->ssao = (uint)settings.ssao_level;
		dRdrList[1]->ssao = (uint)settings.ssao_level;
		dRdrList[2]->ssao = (uint)settings.ssao_level;
		if (settings.ssao_level == level1)
			sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SSAO");
		else if (settings.ssao_level == level2) {
			sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SSAO");
			sManager.getShader(SID_SSAO)->addDef(FSHADER, "SAMPLE_NUM", "4");
			sManager.getShader(SID_SSAO)->reload();
		}
		else if (settings.ssao_level == level3) {
			sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SSAO");
			sManager.getShader(SID_SSAO)->addDef(FSHADER, "SAMPLE_NUM", "16");
			sManager.getShader(SID_SSAO)->reload();
		}
		sManager.getShader(SID_DEFERRED_SHADING)->reload();
	}
	if (settings.smaa_level != stHistory.smaa_level || force_update) {
		stHistory.smaa_level = settings.smaa_level;
		if (settings.smaa_level == level2) {
			sManager.getShader(SID_SMAA_BLENDPASS)->addDef(VSHADER, "MAX_SEARCH_STEPS", "4");
			sManager.getShader(SID_SMAA_BLENDPASS)->addDef(FSHADER, "MAX_SEARCH_STEPS", "4");
		}
		else if (settings.smaa_level == level3) {
			sManager.getShader(SID_SMAA_BLENDPASS)->addDef(VSHADER, "MAX_SEARCH_STEPS", "32");
			sManager.getShader(SID_SMAA_BLENDPASS)->addDef(FSHADER, "MAX_SEARCH_STEPS", "32");
		}
		sManager.getShader(SID_SMAA_BLENDPASS)->reload();
	}
}

void RenderFrame::onLoad() {
	targetFbo = new FrameBuffer(true);
	dRdrList.resize(3);
	dRdrList[0] = new DeferredRenderer(width * values[0][0], height * values[0][0]);
	dRdrList[1] = new DeferredRenderer(width * values[0][1], height * values[0][1]);
	dRdrList[2] = new DeferredRenderer(width, height);
	sFboList.resize(3);
	sFboList[0] = new FrameBuffer;
	sFboList[0]->attachColorTarget(Texture::create(width * values[0][0], height * values[0][0], GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
	sFboList[1] = new FrameBuffer;
	sFboList[1]->attachColorTarget(Texture::create(width * values[0][1], height * values[0][1], GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
	sFboList[2] = new FrameBuffer;
	sFboList[2]->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);

	//oPass = new OmnidirectionalPass(1024, 1024);
	//oPass->setTransforms(glm::vec3(-3.0, 5.0, 0.0), glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, 20.0f));
	//oPass->setShader(sManager.getShader(SID_CUBEMAP_RENDER));

	smaaList.resize(3);
	smaaList[0] = new SMAA(width * values[0][0], height * values[0][0]);
	smaaList[1] = new SMAA(width * values[0][1], height * values[0][1]);
	smaaList[2] = new SMAA(width, height);
	
	resolvePass = new ScreenPass;
	Shader* ups_shader = sManager.getShader(SID_UPSAMPLING);
	resolvePass->setShader(ups_shader);

	{
		scene = new Scene;
		//scene->loadModel("resources/Arcade/Arcade.fbx");
		if (test_scene == 1) {
			/***************************Bistro*********************************/
			scene->loadModel("resources/Bistro/BistroExterior.fbx");
			glm::mat4 modelmat = glm::mat4(1.0);
			modelmat = glm::rotate(modelmat, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
			modelmat = glm::scale(modelmat, glm::vec3(0.003, 0.003, 0.003));
			scene->setModelMat(0, modelmat);
			DirectionalLight* light1 = new DirectionalLight("light1", glm::vec3(3.0, 3.0, 3.0), glm::vec3(0.6, -0.7, -0.3), glm::vec3(-6.0, 7.0, 3.0), 0.03f);
			light1->enableShadow(10.0f, 10.0f, 20.0f, sm_width_list);
			PointLight* light2 = new PointLight("light2", glm::vec3(15.0, 15.0, 15.0), glm::vec3(-3.0, 5.0, 0.0), glm::vec3(0.3, -0.5, 0.0), 0.03f);
			light2->enableShadow(90.0f, 0.5f, 20.0f, sm_width_list);
			scene->addLight(light1);
			scene->addLight(light2);
			light1->addGui(gui);
			light2->addGui(gui);

			camera = new Camera(glm::vec3(-3.0f, 5.0f, 0.0f));
			camera->setPerspective((float)width/(float)height, 0.1f, 100.0f);
			camera->addGui(gui);
			scene->setCamera(camera);
		}
		else if (test_scene == 2) {
			/***************************SunTemple*********************************/
			scene->loadScene("resources/SunTemple/SunTemple.json");
			camera = scene->camera;
			camera->setPerspective((float)width / (float)height, 0.01f, 100.0f);
			scene->camera->addGui(gui);
			scene->dirLights[0]->enableShadow(40.0f, 40.0f, 200.0f, sm_width_list);
			scene->dirLights[0]->addGui(gui);
		}

		std::vector<std::string> skybox_path
		{
			"resources/textures/skybox/posx.jpg",
			"resources/textures/skybox/negx.jpg",
			"resources/textures/skybox/posy.jpg",
			"resources/textures/skybox/negy.jpg",
			"resources/textures/skybox/posz.jpg",
			"resources/textures/skybox/negz.jpg"
		};
		Texture* skybox = Texture::createCubeMapFromFile(skybox_path, false);
		scene->addEnvMap(skybox);
		scene->setEnvMap(0);
	}
	screen->setVisible(true);
	screen->performLayout();

	config(true);
}

void RenderFrame::onFrameRender() {
	glDisable(GL_BLEND); 
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	scene->update();
	targetFbo->clear();

	config();
	duration = glfw->lastFrame - glfw->recordTime;
	if (duration >= 0.25f) {
		fps = (float)(glfw->frame_count - glfw->record_frame) / duration;
		glfw->record();
		time_ratio[0] = scene->record[0].getTime() / duration;
		time_ratio[1] = scene->record[1].getTime() / duration;
		time_ratio[2] = dRenderer->record[0].getTime() / duration;
		time_ratio[3] = dRenderer->record[1].getTime() / duration;
		time_ratio[4] = record[1].getTime() / duration;
	}

	glViewport(0, 0, width * resolution, height * resolution);
	screenFbo->clear();
	dRenderer->renderScene(*scene, *screenFbo);

	record[1].start();
	if(settings.smaa_level != level1)
		smaaPass->Draw(screenFbo->colorAttachs[0].texture);
	glViewport(0, 0, width, height);
	targetFbo->prepare();
	resolvePass->shader->use();
	if (settings.smaa_level == level1)
		resolvePass->shader->setTextureSource("screenTex", 0, screenFbo->colorAttachs[0].texture->id);
	else
		resolvePass->shader->setTextureSource("screenTex", 0, smaaPass->screenBuffer->colorAttachs[0].texture->id);
	resolvePass->render();
	record[1].stop();
}

void RenderFrame::processInput() {
	if (glfw->mouse_enable) {
		if (glfw->checkCursor()) {
			if (moving)camera->ProcessMouseMovement(inputs.mouse_xoffset, inputs.mouse_yoffset);
			screen->cursorPosCallbackEvent(inputs.mouse_xpos, inputs.mouse_ypos);
		}
		if (glfw->checkMouseButton()) {
			if (inputs.mouse_button == GLFW_MOUSE_BUTTON_LEFT && inputs.mouse_action == GLFW_PRESS) moving = true;
			else if (inputs.mouse_button == GLFW_MOUSE_BUTTON_LEFT && inputs.mouse_action == GLFW_RELEASE) moving = false;
			screen->mouseButtonCallbackEvent(inputs.mouse_button, inputs.mouse_action, inputs.mouse_modifiers);
		}
	}
	if (glfw->checkKey())
		screen->keyCallbackEvent(inputs.key, inputs.scancode, inputs.key_action, inputs.key_mods);
	if (glfw->checkChar())
		screen->charCallbackEvent(inputs.codepoint);
	if (glfw->checkDrop())
		screen->dropCallbackEvent(inputs.drop_count, inputs.filenames);
	if (glfw->checkScroll()) {
		camera->ProcessMouseScroll(inputs.scroll_yoffset);
		screen->scrollCallbackEvent(inputs.scroll_xoffset, inputs.scroll_yoffset);
	}
	if (glfw->getKey(GLFW_KEY_W))
		camera->ProcessKeyboard(FORWARD, glfw->deltaTime);
	if (glfw->getKey(GLFW_KEY_S))
		camera->ProcessKeyboard(BACKWARD, glfw->deltaTime);
	if (glfw->getKey(GLFW_KEY_A))
		camera->ProcessKeyboard(LEFT, glfw->deltaTime);
	if (glfw->getKey(GLFW_KEY_D))
		camera->ProcessKeyboard(RIGHT, glfw->deltaTime);
	if (glfw->getKey(GLFW_KEY_Q))
		camera->ProcessKeyboard(RAISE, glfw->deltaTime);
	if (glfw->getKey(GLFW_KEY_E))
		camera->ProcessKeyboard(DROP, glfw->deltaTime);
	if (glfw->getKey(GLFW_KEY_C)) {
		sManager.reload();
	}
}

void RenderFrame::run() {
	while (!glfwWindowShouldClose(glfw->window))
	{
		record[0].start();
		glfw->updateTime();
		glfw->processInput();
		processInput();
		onFrameRender();

		screen->drawContents();
		screen->drawWidgets();
		gui->refresh();

		glfwSwapBuffers(glfw->window);
		glfwPollEvents();
		record[0].stop();
	}
	glfwTerminate();
}