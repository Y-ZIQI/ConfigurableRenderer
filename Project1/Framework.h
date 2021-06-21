#pragma once
#include "AllInclude.h"

const float values[][3] = {
	{0.6f, 0.8f, 1.0f},
	{0.25f, 0.5f, 1.0f}
};
enum Level : int {
	level1 = 0,
	level2,
	level3
};
enum Mode : int {
	mode1 = 0,
	mode2
};
class Settings {
public:
	/** Renderer
	* 1: forward
	* 2: deferred
	*/
	Mode renderer;
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

	Settings() {
		renderer = mode2;
		resolution = level3;
		shadow_resolution = level3;
	}
};

class RenderFrame {
public:
	RenderFrame(int width, int height, const char* title);
	void onLoad();
	void onFrameRender();
	void processInput();
	void config(uint index);
	void run();

	Settings settings;

private:
	int width, height;
	float resolution = 1.0f;

	GLFWENV* glfw;
	Scene* scene;
	Camera* camera;

	ScreenPass* resolvePass;
	ForwardRenderer* fRenderer;
	DeferredRenderer* dRenderer;
	FrameBuffer* targetFbo;
	FrameBuffer* screenFbo;

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
	gui->addGroup("Config");
	gui->addVariable("Renderer", settings.renderer, enabled)->setItems({ "Forward", "Deferred" });
	gui->addVariable("Resolution", settings.resolution, enabled)->setItems({ "60%", "80%", "100%" });
	gui->addVariable("Shadow Resolution", settings.shadow_resolution, enabled)->setItems({ "512", "1024", "2048" });
}

void RenderFrame::config(uint index) {
	if (index == 0) {
		resolution = values[0][settings.resolution];
	}
	else if (index == 1) {
		float shadow_resolution = values[1][settings.shadow_resolution];
		for (int i = 0; i < scene->dirLights.size(); i++)
			scene->dirLights[i]->setResolution(shadow_resolution);
		for (int i = 0; i < scene->ptLights.size(); i++)
			scene->ptLights[i]->setResolution(shadow_resolution);
	}
}

void RenderFrame::onLoad() {
	dRenderer = new DeferredRenderer(width, height);
	fRenderer = new ForwardRenderer();
	targetFbo = new FrameBuffer(true);
	screenFbo = new FrameBuffer;
	screenFbo->attachColorTarget(Texture::create(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR), 0);
	screenFbo->addDepthStencil(width, height);

	//oPass = new OmnidirectionalPass(1024, 1024);
	//oPass->setTransforms(glm::vec3(-3.0, 5.0, 0.0), glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, 20.0f));
	//oPass->setShader(sManager.getShader(SID_CUBEMAP_RENDER));
	
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
			light1->enableShadow(10.0f, 10.0f, 20.0f, 2048, 2048);
			PointLight* light2 = new PointLight("light2", glm::vec3(15.0, 15.0, 15.0), glm::vec3(-3.0, 5.0, 0.0), glm::vec3(0.3, -0.5, 0.0), 0.03f);
			light2->enableShadow(90.0f, 0.5f, 20.0f, 2048, 2048);
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
			scene->loadModel("resources/SunTemple/SunTemple.fbx");
			glm::mat4 modelmat = glm::mat4(1.0);
			modelmat = glm::scale(modelmat, glm::vec3(0.01, 0.01, 0.01));
			scene->setModelMat(0, modelmat);
			DirectionalLight* light1 = new DirectionalLight("SunLight", glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.86f, -0.12f, 0.50f), glm::vec3(-8.60f, 1.20f, -5.0f), 0.05f);
			PointLight* light2 = new PointLight("FirePitBackHall1", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(1.64f, 2.45f, 70.86f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			//PointLight* light3 = new PointLight("FirePitBackHall2", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(-1.45f, 2.45f, 70.86f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			//PointLight* light4 = new PointLight("FirePitBackHall3", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(-2.09f, 2.45f, 50.81f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			//PointLight* light5 = new PointLight("FireBowlBackHall", glm::vec3(1.0f, 0.08f, 0.0f), glm::vec3(0.0f, 1.72f, 58.02f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			//PointLight* light6 = new PointLight("FirePitBackHallEntrance1", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(-7.14f, 2.45f, 43.18f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			PointLight* light3 = new PointLight("FirePitFront1", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(7.90f, 4.41f, -2.15f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			PointLight* light4 = new PointLight("FirePitFront2", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(2.11f, 4.41f, -7.97f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			PointLight* light5 = new PointLight("FirePitFront3", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(-7.98f, 4.41f, -2.06f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			PointLight* light6 = new PointLight("FirePitFront4", glm::vec3(1.0f, 0.24f, 0.08f), glm::vec3(-3.10f, 4.41f, 7.86f), glm::vec3(0.0f, -1.0f, 0.0f), 0.05f, 20.0f);
			scene->addLight(light1);
			light1->enableShadow(40.0f, 40.0f, 200.0f, 2048, 2048);
			scene->addLight(light2);
			scene->addLight(light3);
			scene->addLight(light4);
			scene->addLight(light5);
			scene->addLight(light6);
			light1->addGui(gui);

			camera = new Camera(glm::vec3(0.0f, 3.2f, 71.1f));
			camera->setPerspective((float)width / (float)height, 0.01f, 100.0f);
			camera->addGui(gui);
			scene->setCamera(camera);
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
}

void RenderFrame::onFrameRender() {
	glDisable(GL_BLEND); 
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	scene->update();
	targetFbo->clear();

	config(0);
	config(1);
	duration = glfw->lastFrame - glfw->recordTime;
	if (duration >= 0.25f) {
		fps = (float)(glfw->frame_count - glfw->record_frame) / duration;
		glfw->record();
	}

	//oPass->renderScene(*scene, resolution);
	
	if (settings.renderer == mode1) {
		glViewport(0, 0, width * resolution, height * resolution);
		screenFbo->clear();
		fRenderer->renderScene(*scene, *screenFbo);
	}
	else if (settings.renderer == mode2) {
		screenFbo->clear();
		dRenderer->renderScene(*scene, *screenFbo, resolution);
	}
	glViewport(0, 0, width, height);
	targetFbo->prepare();
	resolvePass->shader->use();
	resolvePass->shader->setFloat("resolution", resolution);
	resolvePass->shader->setTextureSource("screenTex", 0, screenFbo->colorAttachs[0].texture->id);
	resolvePass->render();
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
		glfw->updateTime();
		glfw->processInput();
		processInput();
		onFrameRender();

		screen->drawContents();
		screen->drawWidgets();
		gui->refresh();

		glfwSwapBuffers(glfw->window);
		glfwPollEvents();
	}
	glfwTerminate();
}