#pragma once
#include <windows.h>
#include "defines.h"

struct InputInfo {
    bool cursor_changed, mbutton_changed, key_changed, char_changed, drop_changed, scroll_changed;
    double mouse_xpos, mouse_ypos;
    double mouse_xoffset, mouse_yoffset;
    int mouse_button, mouse_action, mouse_modifiers;
    int key, scancode, key_action, key_mods;
    int codepoint;
    int drop_count;
    const char** filenames;
    double scroll_xoffset, scroll_yoffset;
};

// Inputs are accessible anywhere
InputInfo inputs{ false, false, false, false };

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double lastX, lastY;
    static bool firstMouse = true;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    inputs.mouse_xpos = xpos;
    inputs.mouse_ypos = ypos;
    inputs.mouse_xoffset = xpos - lastX;
    inputs.mouse_yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    inputs.cursor_changed = true;
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int modifiers) {
    inputs.mouse_button = button;
    inputs.mouse_action = action;
    inputs.mouse_modifiers = modifiers;
    inputs.mbutton_changed = true;
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    inputs.key = key; inputs.scancode = scancode; inputs.key_action = action; inputs.key_mods = mods;
    inputs.key_changed = true;
}
void char_callback(GLFWwindow* window, unsigned int codepoint) {
    inputs.codepoint = codepoint;
    inputs.char_changed = true;
}
void drop_callback(GLFWwindow* window, int count, const char** filenames) {
    inputs.drop_count = count; inputs.filenames = filenames;
    inputs.drop_changed = true;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    inputs.scroll_xoffset = xoffset; inputs.scroll_yoffset = yoffset;
    inputs.scroll_changed = true;
}

class GLFWENV {
public:
    uint width, height;
    uint frame_count = 0, record_frame = 0;
    float deltaTime = 0.0f, lastFrame = 0.0f, recordTime = 0.0f;
    GLFWwindow* window;
    bool mouse_enable = true;

    GLFWENV(uint width, uint height, const char* title, bool mouse_enable = true) {
        this->width = width;
        this->height = height;
        this->mouse_enable = mouse_enable;
        this->init(title);
        this->setConfig();
    };
    void updateTime() {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        frame_count++;
    };
    void record() {
        record_frame = frame_count;
        recordTime = lastFrame;
    }
    bool getKey(int key, int state = GLFW_PRESS) {
        return glfwGetKey(window, key) == state;
    };
    bool checkCursor() { return inputs.cursor_changed ? !(inputs.cursor_changed = false) : false; };
    bool checkMouseButton() { return inputs.mbutton_changed ? !(inputs.mbutton_changed = false) : false; };
    bool checkKey() { return inputs.key_changed ? !(inputs.key_changed = false) : false; };
    bool checkChar() { return inputs.char_changed ? !(inputs.char_changed = false) : false; };
    bool checkDrop() { return inputs.drop_changed ? !(inputs.drop_changed = false) : false; };
    bool checkScroll() { return inputs.scroll_changed ? !(inputs.scroll_changed = false) : false; };
    void processInput() {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    };

private:
    void init(const char* title) {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
        }
        glfwMakeContextCurrent(window);

        // glad: load all OpenGL function pointers
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
        }
    }
    void setConfig() {
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetCharCallback(window, char_callback);
        glfwSetDropCallback(window, drop_callback);
        glfwSetScrollCallback(window, scroll_callback);

        // Turn off Vertical synchronization
        /*typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALFARPROC)(int);
        PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
        wglSwapIntervalEXT(0);*/

        if (mouse_enable)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &_max_anisotropy);
    }
};