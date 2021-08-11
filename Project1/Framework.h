#pragma once
#include "AllInclude.h"

const char* demo_scene_paths[] = {
    "resources/Bistro/BistroExterior.json",
    "resources/Bistro/BistroInterior_Wine.json",
    "resources/SunTemple/SunTemple.json",
    "resources/Arcade/Arcade.json"
};

const float values[][3] = {
    {0.6f, 0.8f, 1.0f},
    {0.25f, 0.5f, 1.0f}
};
const char* shadow_type_name[3] = {
    "SHADOW_SOFT_EVSM", "SHADOW_SOFT_PCSS", "SHADOW_HARD"
};
const uint shadow_type_tex_id[3] = {1, 0, 0};
const char* level_name[4] = {
    "0", "1", "2", "3"
};
enum Level : uint {
    level1 = 0,
    level2,
    level3
};
enum Mode : uint {
    mode1 = 0,
    mode2,
    mode3,
    mode4
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

    bool recording = true;
    // 1: Not update, 2: As need, 3: Every frame
    Mode update_shadow;
    // 1: evsm, 2: pcss, 3: hard
    Mode shadow_type;
    Level effect_level;

    Settings() {
        resolution = level3;
        shadow_resolution = level3;
        ssao_level = level1;
        ssr_level = level1;
        shading = level1;
        smaa_level = level1;

        update_shadow = mode2;
        shadow_type = mode1;
        effect_level = level2;
    }
};

class RenderFrame {
public:
    RenderFrame(int width, int height, const char* title);
    ~RenderFrame() {};
    void onLoad();
    void onFrameRender();
    void onGuiRender();
    void onDestroy();
    void loadScene(string const& path, const string name = "");
    void processInput();
    uint config(bool force_update = false);
    void initResources();
    void update();
    void run();

    Settings settings, stHistory;

private:
    int width, height;
    GLubyte* image_data;

    GLFWENV* glfw;
    Scene* scene = nullptr;
    Camera* camera;

    std::vector<DeferredRenderer*> dRdrList;
    std::vector<SMAA*> smaaList;
    DeferredRenderer* dRenderer;
    Shader* resolveShader;
    FrameBuffer* screenFbo;
    SMAA* smaaPass;
    FrameBuffer* targetFbo; // Respect for default FB

    OmnidirectionalPass* oPass;

    bool moving = false;

    /**
    * 0: Bistro
    * 1: Bistro Interior
    * 2: SunTemple
    * 3: Arcade
    */
    uint test_scene = 3;
    float fps, duration;
};

RenderFrame::RenderFrame(int width, int height, const char* title) {
    this->width = width;
    this->height = height;
    image_data = new GLubyte[width * height * 3];
    glfw = new GLFWENV(width, height, title, true);

    frame_record.init();
    sManager.init();
    tManager.init();
}

void RenderFrame::loadScene(string const& path, const string name) {
    scene->loadScene(path, name);
    camera = scene->camera;
    camera->setAspect((float)width / (float)height);
    initResources();
}

void RenderFrame::onLoad() {
    {
        targetFbo = new FrameBuffer(true);
        dRdrList.resize(3);
        dRdrList[0] = new DeferredRenderer(width * values[0][0], height * values[0][0]);
        dRdrList[1] = new DeferredRenderer(width * values[0][1], height * values[0][1]);
        dRdrList[2] = new DeferredRenderer(width, height);

        oPass = new OmnidirectionalPass(512, 512);

        smaaList.resize(3);
        smaaList[0] = new SMAA(width * values[0][0], height * values[0][0]);
        smaaList[1] = new SMAA(width * values[0][1], height * values[0][1]);
        smaaList[2] = new SMAA(width, height);

        resolveShader = sManager.getShader(SID_UPSAMPLING);
    }
    scene = new Scene;
    loadScene(demo_scene_paths[test_scene]);
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();
        ImGui_ImplGlfw_InitForOpenGL(glfw->window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
    }
}

void RenderFrame::initResources() {
    for (uint i = 0; i < scene->light_probes.size(); i++) {
        if (!scene->light_probes[i]->processed) {
            oPass->setTarget(scene->light_probes[i]->texCube, scene->light_probes[i]->depthCube);
            oPass->setTransforms(scene->light_probes[i]->position, glm::perspective(glm::radians(90.0f), 1.0f, scene->light_probes[i]->nearz, scene->light_probes[i]->farz));
            oPass->blitEnvmap(scene->envMaps[0]);
            oPass->renderScene(*scene);
            scene->light_probes[i]->filter(5);
            scene->light_probes[i]->processed = true;
        }
    }
}

void RenderFrame::onFrameRender() {
    update();
    if (scene) {
        uint flag = config(scene->getLoadedStatue());
        scene->update((flag & 0x2) ? 2 : settings.update_shadow, settings.shadow_type == mode1);
        dRenderer->renderScene(*scene);
        if (settings.smaa_level != level1)
            smaaPass->Draw(screenFbo->colorAttachs[0].texture);
        glViewport(0, 0, width, height);
        resolveShader->use();
        if (settings.smaa_level == level1)
            resolveShader->setTextureSource("screenTex", 0, screenFbo->colorAttachs[0].texture->id);
        else
            resolveShader->setTextureSource("screenTex", 0, smaaPass->screenBuffer->colorAttachs[0].texture->id);
        targetFbo->prepare();
        targetFbo->clear();
        renderScreen();
    }
    else {
        targetFbo->prepare();
        targetFbo->clear(ALL_TARGETS, _clear_color_blue);
    }
    if (settings.recording) {
        if(glfw->frame_count > 1) 
            frame_record.get();
        frame_record.copy();
    }
    CHECKERROR
}

void RenderFrame::update(){
    frame_record.clear(settings.recording);
    glfw->updateTime();
    duration = glfw->lastFrame - glfw->recordTime;
    if (duration >= 0.25f) {
        fps = (float)(glfw->frame_count - glfw->record_frame) / duration;
        glfw->record();
    }
}

void RenderFrame::onGuiRender() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = false;
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
    {
        ImGui::Begin("Demo");
        if(glfw->frame_count <= 1) ImGui::SetNextItemOpen(true);
        if (ImGui::CollapsingHeader("Status")) {
            ImGui::Text("Average time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Triangles amount: %d", frame_record.triangles);
            ImGui::Text("Draw Calls amount: %d", frame_record.draw_calls);
            ImGui::Text("Texture Samples: %d", frame_record.texture_samples);
            ImGui::Checkbox("Recording", &settings.recording);
        }
        if (glfw->frame_count <= 1) ImGui::SetNextItemOpen(true);
        if (ImGui::CollapsingHeader("Config")) {
            static const char* elems_names1[] = { "PBR", "PBR+IBL" };
            static const char* elems_names2[] = { "60%%", "80%%", "100%%" };
            static const char* elems_names3[] = { "25%%", "50%%", "100%%" };
            static const char* elems_names4[] = { "Disable", "Low", "High" };
            ImGui::Combo("Shading", (int*)&settings.shading, elems_names1, 2);
            ImGui::SliderInt("Resolution", (int*)&settings.resolution, 0, 2, elems_names2[settings.resolution]);
            ImGui::SliderInt("Shadow Resolution", (int*)&settings.shadow_resolution, 0, 2, elems_names3[settings.shadow_resolution]);
            ImGui::SliderInt("SSAO Level", (int*)&settings.ssao_level, 0, 2, elems_names4[settings.ssao_level]);
            ImGui::SliderInt("SSR Level", (int*)&settings.ssr_level, 0, 2, elems_names4[settings.ssr_level]);
            ImGui::SliderInt("SMAA Level", (int*)&settings.smaa_level, 0, 2, elems_names4[settings.smaa_level]);
            ImGui::SliderInt("Effect", (int*)&settings.effect_level, 0, 2, elems_names4[settings.effect_level]);
        }
        if (glfw->frame_count <= 1) ImGui::SetNextItemOpen(true);
        if (ImGui::CollapsingHeader("Render Setting")) {
            static const char* elems_names5[] = { "Not update", "As need", "Every frame" };
            static const char* elems_names6[] = { "EVSM", "PCSS", "Hard" };
            ImGui::Combo("Update Shadow", (int*)&settings.update_shadow, elems_names5, 3);
            ImGui::Combo("Shadow Type", (int*)&settings.shadow_type, elems_names6, 3);
        }
        if (ImGui::CollapsingHeader("Scene")) {
            static const char* elems_names7[] = { "Bistro", "Bistro Interior", "SunTemple", "Arcade" };
            ImGui::Combo("Demo Scenes", (int*)&test_scene, elems_names7, 4);
            if (ImGui::Button("Load Scene")) {
                loadScene(demo_scene_paths[test_scene]);
            }
            scene->renderGui();
        }
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::End();
    }
    ImGui::Render();
    glViewport(0, 0, width, height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

uint RenderFrame::config(bool force_update) {
    uint flag = 0x0000;
    if (settings.resolution != stHistory.resolution || force_update) {
        flag |= 0x1;
        stHistory.resolution = settings.resolution;
        dRenderer = dRdrList[settings.resolution];
        screenFbo = dRenderer->screenBuffer;
        smaaPass = smaaList[settings.resolution];
    }
    if (settings.shadow_resolution != stHistory.shadow_resolution || force_update) {
        flag |= 0x2;
        stHistory.shadow_resolution = settings.shadow_resolution;
        for (int i = 0; i < scene->dirLights.size(); i++)
            scene->dirLights[i]->setShadowMap(settings.shadow_resolution);
        for (int i = 0; i < scene->ptLights.size(); i++)
            scene->ptLights[i]->setShadowMap(settings.shadow_resolution);
        for (int i = 0; i < scene->radioLights.size(); i++)
            scene->radioLights[i]->setShadowMap(settings.shadow_resolution);
    }
    if (settings.shading != stHistory.shading || force_update) {
        flag |= 0x4;
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
        flag |= 0x8;
        stHistory.ssao_level = settings.ssao_level;
        dRdrList[0]->ssao = (uint)settings.ssao_level;
        dRdrList[1]->ssao = (uint)settings.ssao_level;
        dRdrList[2]->ssao = (uint)settings.ssao_level;
        if (settings.ssao_level == level1) {
            sManager.getShader(SID_DEFERRED_PREPROCESS)->removeDef(FSHADER, "SSAO");
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "SSAO");
        }
        else if (settings.ssao_level == level2) {
            sManager.getShader(SID_DEFERRED_PREPROCESS)->addDef(FSHADER, "SSAO");
            sManager.getShader(SID_DEFERRED_PREPROCESS)->addDef(FSHADER, "SAMPLE_NUM", "4");
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SSAO");
        }
        else if (settings.ssao_level == level3) {
            sManager.getShader(SID_DEFERRED_PREPROCESS)->addDef(FSHADER, "SSAO");
            sManager.getShader(SID_DEFERRED_PREPROCESS)->addDef(FSHADER, "SAMPLE_NUM", "16");
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "SSAO");
        }
        sManager.getShader(SID_DEFERRED_PREPROCESS)->reload();
        sManager.getShader(SID_DEFERRED_SHADING)->reload();
    }
    if (settings.ssr_level != stHistory.ssr_level || force_update) {
        flag |= 0x10;
        stHistory.ssr_level = settings.ssr_level;
        dRdrList[0]->ssr = (uint)settings.ssr_level;
        dRdrList[1]->ssr = (uint)settings.ssr_level;
        dRdrList[2]->ssr = (uint)settings.ssr_level;
        if (settings.ssr_level != level1) {
            sManager.getShader(SID_SSR_RAYTRACE)->addDef(FSHADER, "SSR_LEVEL", level_name[(uint)settings.ssr_level]);
            sManager.getShader(SID_SSR_RESOLVE)->addDef(FSHADER, "SSR_LEVEL", level_name[(uint)settings.ssr_level]);
            sManager.getShader(SID_SSR_RAYTRACE)->reload();
            sManager.getShader(SID_SSR_RESOLVE)->reload();
        }
    }
    if (settings.smaa_level != stHistory.smaa_level || force_update) {
        flag |= 0x20;
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
        flag |= 0x2;
        stHistory.shadow_type = settings.shadow_type;
        for (uint i = mode1; i <= mode3; i++) {
            if (i == settings.shadow_type) {
                sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, shadow_type_name[i]);
                sManager.getShader(SID_SHADOWMAP)->addDef(FSHADER, shadow_type_name[i]);
                sManager.getShader(SID_OMNISHADOWMAP)->addDef(FSHADER, shadow_type_name[i]);
            }
            else {
                sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, shadow_type_name[i]);
                sManager.getShader(SID_SHADOWMAP)->removeDef(FSHADER, shadow_type_name[i]);
                sManager.getShader(SID_OMNISHADOWMAP)->removeDef(FSHADER, shadow_type_name[i]);
            }
        }
        sManager.getShader(SID_DEFERRED_SHADING)->reload();
        sManager.getShader(SID_SHADOWMAP)->reload();
        sManager.getShader(SID_OMNISHADOWMAP)->reload();
        for (int i = 0; i < scene->dirLights.size(); i++)
            scene->dirLights[i]->setShadowMapTex(shadow_type_tex_id[settings.shadow_type]);
        for (int i = 0; i < scene->ptLights.size(); i++)
            scene->ptLights[i]->setShadowMapTex(shadow_type_tex_id[settings.shadow_type]);
        for (int i = 0; i < scene->radioLights.size(); i++)
            scene->radioLights[i]->setShadowMapTex(shadow_type_tex_id[settings.shadow_type]);
    }
    if (settings.effect_level != stHistory.effect_level || force_update) {
        flag |= 0x40;
        stHistory.effect_level = settings.effect_level;
        dRdrList[0]->effect = (uint)settings.effect_level;
        dRdrList[1]->effect = (uint)settings.effect_level;
        dRdrList[2]->effect = (uint)settings.effect_level;
        if (settings.effect_level == level1) {
            sManager.getShader(SID_DEFERRED_SHADING)->removeDef(FSHADER, "BLOOM");
        }
        else if (settings.effect_level == level2) {
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "BLOOM");
            sManager.getShader(SID_BLOOM_BLUR)->removeDef(FSHADER, "BLOOM_HIGH");
            sManager.getShader(SID_BLOOM_BLUR)->reload();
        }
        else if (settings.effect_level == level3) {
            sManager.getShader(SID_DEFERRED_SHADING)->addDef(FSHADER, "BLOOM");
            sManager.getShader(SID_BLOOM_BLUR)->addDef(FSHADER, "BLOOM_HIGH");
            sManager.getShader(SID_BLOOM_BLUR)->reload();
        }
        sManager.getShader(SID_DEFERRED_SHADING)->reload();
    }
    return flag;
}

void RenderFrame::processInput() {
    if (glfw->mouse_enable) {
        if (glfw->checkCursor()) {
            if (moving && !ImGui::GetIO().WantCaptureMouse)
                camera->ProcessMouseMovement(inputs.mouse_xoffset, inputs.mouse_yoffset);
        }
        if (glfw->checkMouseButton()) {
            if (inputs.mouse_button == GLFW_MOUSE_BUTTON_LEFT && inputs.mouse_action == GLFW_PRESS) 
                moving = true;
            else if (inputs.mouse_button == GLFW_MOUSE_BUTTON_LEFT && inputs.mouse_action == GLFW_RELEASE) 
                moving = false;
        }
    }
    if (glfw->checkScroll()) {
        if (!ImGui::GetIO().WantCaptureMouse)
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
        // Reload all shaders (update changes)
        sManager.reload();
    }
    if (glfw->getKey(GLFW_KEY_X)) {
        // Print screen into `img/newimage`, then you can run `img/genImage.py` to generate .png image
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image_data);
        std::ofstream ofs;
        ofs.open("img/newimage", std::ios_base::out | std::ios_base::binary);
        char wh[4] = { width / 100, width % 100, height / 100, height % 100 };
        ofs.write((const char*)wh, 4);
        ofs.write((const char*)image_data, width * height * 3);
        ofs.close();
    }
}

void RenderFrame::run() {
    while (!glfwWindowShouldClose(glfw->window))
    {
        glfw->processInput();
        processInput();
        onFrameRender();
        onGuiRender();

        glfwSwapBuffers(glfw->window);
        glfwPollEvents();
    }
    glfwTerminate();
}

void RenderFrame::onDestroy() {
    if (scene)delete scene;
}