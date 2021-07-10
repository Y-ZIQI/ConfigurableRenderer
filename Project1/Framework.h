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
    mode2,
    mode3
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
    /** SSR level
    * 1: no SSR
    * 2: SSR
    * 3: TODO
    */
    Level ssr_level;
    /** Shading
    * 1: PBR
    * 2: PBR + IBL
    * 3: TODO: 
    */
    Level shading;
    /** SMAA level
    * 1: no SMAA
    * 2: search step 4
    * 3: search step 32
    */
    Level smaa_level;

    bool update_shadow;
    bool recording = true;
    // 1: hard, 2: pcf, 3: esm
    Mode shadow_type;

    Settings() {
        resolution = level3;
        shadow_resolution = level3;
        ssao_level = level1;
        ssr_level = level1;
        shading = level1;
        smaa_level = level1;

        update_shadow = true;
        shadow_type = mode2;
    }
};

class RenderFrame {
public:
    RenderFrame(int width, int height, const char* title);
    void onLoad();
    void onFrameRender();
    void processInput();
    void config(bool force_update = false);
    void update();
    void run();

    Settings settings, stHistory;

private:
    int width, height;
    float resolution = 1.0f;

    GLFWENV* glfw;
    Scene* scene;
    Camera* camera;

    std::vector<DeferredRenderer*> dRdrList;
    std::vector<SMAA*> smaaList;
    DeferredRenderer* dRenderer;
    ScreenPass* resolvePass;
    FrameBuffer* screenFbo;
    SMAA* smaaPass;
    FrameBuffer* targetFbo; // Respect for default FB

    OmnidirectionalPass* oPass;
    
    nanogui::Screen* screen = nullptr;
    nanogui::FormHelper* gui;
    nanogui::ref<nanogui::Window> mainWindow, hideWindow;
    //nanogui::ref<nanogui::Window> sceneWindow;
    uint win_id = 0;

    bool enabled = true;
    bool moving = false;

    /**
    * 1: Bistro
    * 2: SunTemple
    */
    uint test_scene = 1;
    float fps, duration;
    TimeRecord record[2]; // All, SMAA+resolve
    float time_ratio[5]; // Shadow, Draw, AO, Shading, Post
};

RenderFrame::RenderFrame(int width, int height, const char* title) {
    this->width = width;
    this->height = height;
    glfw = new GLFWENV(width, height, title, true);

    frame_record.init();
    sManager.init();
    tManager.init();

    screen = new nanogui::Screen();
    screen->initialize(glfw->window, true);
}

void RenderFrame::onLoad() {
    {
        targetFbo = new FrameBuffer(true);
        dRdrList.resize(3);
        dRdrList[0] = new DeferredRenderer(width * values[0][0], height * values[0][0]);
        dRdrList[1] = new DeferredRenderer(width * values[0][1], height * values[0][1]);
        dRdrList[2] = new DeferredRenderer(width, height);

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
    }
    {
        scene = new Scene;
        if (test_scene == 1) {
            /***************************Bistro*********************************/
            scene->loadScene("resources/Bistro/BistroExterior.json", "Bistro");
            scene->dirLights[0]->enableShadow(80.0f, 80.0f, 200.0f, sm_width_list);
            scene->ptLights[0]->enableShadow(90.0f, 1.0f, 200.0f, sm_width_list);
        }
        else if (test_scene == 2) {
            /***************************SunTemple*********************************/
            scene->loadScene("resources/SunTemple/SunTemple.json", "SunTemple");
            scene->dirLights[0]->enableShadow(40.0f, 40.0f, 200.0f, sm_width_list);
        }
        camera = scene->camera;
        camera->setAspect((float)width / (float)height);
    }
    {
        gui = new nanogui::FormHelper(screen);
        hideWindow = gui->addWindow(Eigen::Vector2i(0, 0), "Demo");
        hideWindow->setVisible(false);
        gui->addButton("Show Panel", [this]() { hideWindow->setVisible(false); mainWindow->setVisible(true); })->setIcon(ENTYPO_ICON_CHEVRON_DOWN);
        mainWindow = gui->addWindow(Eigen::Vector2i(0, 0), "Demo");
        mainWindow->setWidth(250);
        gui->addButton("Hide Panel", [this]() { hideWindow->setVisible(true); mainWindow->setVisible(false); })->setIcon(ENTYPO_ICON_CHEVRON_UP);
        gui->addGroup("Status");
        gui->addVariable("FPS", fps, false);
        gui->addVariable("Triangles", frame_record.triangles, false);
        gui->addVariable("Draw Calls", frame_record.draw_calls, false);
        gui->addVariable("Texture Samples", frame_record.texture_samples, false);
        gui->addGroup("Config");
        gui->addVariable("Resolution", settings.resolution, enabled)->setItems({ "60%", "80%", "100%" });
        gui->addVariable("Shadow Resolution", settings.shadow_resolution, enabled)->setItems({ "512", "1024", "2048" });
        gui->addVariable("Shading", settings.shading, enabled)->setItems({ "PBR", "PBR+IBL", "3" });
        gui->addVariable("SSAO Level", settings.ssao_level, enabled)->setItems({ "Disable", "1", "2" });
        gui->addVariable("SSR Level", settings.ssr_level, enabled)->setItems({ "Disable", "1", "2" });
        gui->addVariable("SMAA Level", settings.smaa_level, enabled)->setItems({ "Disable", "1", "2" });
        gui->addGroup("Render Setting");
        gui->addVariable("Recording", settings.recording, enabled);
        gui->addVariable("Update Shadow", settings.update_shadow, enabled);
        gui->addVariable("Shadow Type", settings.shadow_type, enabled)->setItems({ "Hard", "PCF", "ESM" });
        gui->addGroup("Scenes");
        scene->addGui(gui, mainWindow);
        screen->setVisible(true);
        screen->performLayout();
    }
    config(true);
}

void RenderFrame::onFrameRender() {
    glDisable(GL_BLEND); 
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    config();
    update();

    scene->update(settings.update_shadow);
    targetFbo->clear();

    dRenderer->renderScene(*scene);

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

    frame_record.triangles += 2;
    frame_record.draw_calls += 1;
}

void RenderFrame::update(){
    duration = glfw->lastFrame - glfw->recordTime;
    if (duration >= 0.25f) {
        fps = (float)(glfw->frame_count - glfw->record_frame) / duration;
        glfw->record();
        time_ratio[0] = record[0].getTime() / duration;
    }
    frame_record.clear(settings.recording);
}

void RenderFrame::config(bool force_update) {
    if (settings.resolution != stHistory.resolution || force_update) {
        stHistory.resolution = settings.resolution;
        resolution = values[0][settings.resolution];
        dRenderer = dRdrList[settings.resolution];
        screenFbo = dRenderer->screenBuffer;
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
        dRdrList[0]->ibl = (uint)settings.shading;
        dRdrList[1]->ibl = (uint)settings.shading;
        dRdrList[2]->ibl = (uint)settings.shading;
        if (settings.shading == level1)
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "EVAL_IBL");
        else if (settings.shading == level2)
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "EVAL_IBL");
        else if (settings.shading == level3);
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
    if (settings.ssr_level != stHistory.ssr_level || force_update) {
        stHistory.ssr_level = settings.ssr_level;
        dRdrList[0]->ssr = (uint)settings.ssr_level;
        dRdrList[1]->ssr = (uint)settings.ssr_level;
        dRdrList[2]->ssr = (uint)settings.ssr_level;
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
    if (settings.shadow_type != stHistory.shadow_type || force_update) {
        stHistory.shadow_type = settings.shadow_type;
        if (settings.shadow_type == mode1) {
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SHADOW_SOFT_PCF");
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SHADOW_SOFT_ESM");
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SHADOW_HARD");
            sManager.getShader(SID_SHADOWMAP)->removeDef(FSHADER, "SHADOW_SOFT_ESM");
        }
        else if (settings.shadow_type == mode2) {
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SHADOW_HARD");
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SHADOW_SOFT_ESM");
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SHADOW_SOFT_PCF");
            sManager.getShader(SID_SHADOWMAP)->removeDef(FSHADER, "SHADOW_SOFT_ESM");
        }
        else if (settings.shadow_type == mode3) {
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SHADOW_SOFT_PCF");
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SHADOW_HARD");
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SHADOW_SOFT_ESM");
            sManager.getShader(SID_SHADOWMAP)->addDef(FSHADER, "SHADOW_SOFT_ESM");
        }
        sManager.getShader(SID_DEFERRED_SHADING)->reload();
        sManager.getShader(SID_SHADOWMAP)->reload();
    }
}

void RenderFrame::processInput() {
    if (glfw->mouse_enable) {
        if (glfw->checkCursor()) {
            if (moving)camera->ProcessMouseMovement(inputs.mouse_xoffset, inputs.mouse_yoffset);
            screen->cursorPosCallbackEvent(inputs.mouse_xpos, inputs.mouse_ypos);
        }
        if (glfw->checkMouseButton()) {
            if (!screen->mouseButtonCallbackEvent(inputs.mouse_button, inputs.mouse_action, inputs.mouse_modifiers) && 
                inputs.mouse_button == GLFW_MOUSE_BUTTON_LEFT &&
                inputs.mouse_action == GLFW_PRESS) moving = true;
            else if (inputs.mouse_button == GLFW_MOUSE_BUTTON_LEFT && inputs.mouse_action == GLFW_RELEASE) moving = false;
        }
    }
    if (glfw->checkKey())
        screen->keyCallbackEvent(inputs.key, inputs.scancode, inputs.key_action, inputs.key_mods);
    if (glfw->checkChar())
        screen->charCallbackEvent(inputs.codepoint);
    if (glfw->checkDrop())
        screen->dropCallbackEvent(inputs.drop_count, inputs.filenames);
    if (glfw->checkScroll()) {
        if (!screen->scrollCallbackEvent(inputs.scroll_xoffset, inputs.scroll_yoffset))
            camera->ProcessMouseScroll(inputs.scroll_yoffset);
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
        record[0].stop();
        record[0].start();
        glfw->updateTime();
        glfw->processInput();
        processInput();
        onFrameRender();

        screen->drawContents();
        screen->drawWidgets();
        gui->refresh();

        if (settings.recording) {
            frame_record.copy();
            frame_record.get();
        }
        glfwSwapBuffers(glfw->window);
        glfwPollEvents();
    }
    glfwTerminate();
}