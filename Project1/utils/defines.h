#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nanogui/nanogui.h>
#include <jsonxx/json.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <random>

typedef unsigned int uint;
typedef unsigned char uchar;

#define M_PI                3.1415926536f  // pi
#define M_2PI               6.2831853072f  // 2pi
#define M_1_PI              0.3183098862f // 1/pi

GLfloat _max_anisotropy;

#define MAX_TARGETS 8
#define ALL_TARGETS -2
const GLuint _color_attachments[MAX_TARGETS] = {
    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7
};
const GLfloat _clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
const GLfloat _clear_color_1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

#define VSHADER 0
#define FSHADER 1
#define GSHADER 2
#define SID_DEFERRED_BASE           0
#define SID_DEFERRED_SHADING        1
#define SID_DEFERRED_ENVMAP         2
#define SID_DEFERRED_ENVMAP2D       3
#define SID_UPSAMPLING              4
#define SID_SHADOWMAP               5
#define SID_CUBEMAP_RENDER          6
#define SID_SSAO                    7
#define SID_SMAA_EDGEPASS           8
#define SID_SMAA_BLENDPASS          9
#define SID_SMAA_NEIGHBORPASS       10
#define SID_SSR                     11
#define SID_IBL_CONVOLUTION         12
const std::vector<const char*> _shader_paths
{
    /* 0*/"shaders/deferred/basePass.vs", "shaders/deferred/basePass.fs", nullptr,
    /* 1*/"shaders/deferred/shadingPass.vs", "shaders/deferred/shadingPass.fs", nullptr,
    /* 2*/"shaders/envmap/deferred.vs", "shaders/envmap/deferred.fs", nullptr,
    /* 3*/"shaders/envmap/deferred2d.vs", "shaders/envmap/deferred2d.fs", nullptr,
    /* 4*/"shaders/upsampling/main.vs", "shaders/upsampling/main.fs", nullptr,
    /* 5*/"shaders/shadow/shadow.vs", "shaders/shadow/shadow.fs", nullptr,
    /* 6*/"shaders/omnidirectional/main.vs", "shaders/omnidirectional/main.fs", "shaders/omnidirectional/main.gs",
    /* 7*/"shaders/ssao/ssao.vs", "shaders/ssao/ssao.fs", nullptr,
    /* 8*/"shaders/smaa/edgeDetection.vs", "shaders/smaa/edgeDetection.fs", nullptr,
    /* 9*/"shaders/smaa/blendCalculation.vs", "shaders/smaa/blendCalculation.fs", nullptr,
    /*10*/"shaders/smaa/neighborBlending.vs", "shaders/smaa/neighborBlending.fs", nullptr,
    /*11*/"shaders/ssr/ssr.vs", "shaders/ssr/ssr.fs", nullptr,
    /*12*/"shaders/ibl/convolution.vs", "shaders/ibl/convolution.fs", nullptr
};
//TODO: add default defines
const std::vector<std::initializer_list<std::pair<const std::string, std::string>>> _shader_defs
{
    /* 0*/{}, {}, {},
    /* 1*/{}, {}, {},
    /* 2*/{}, {}, {},
    /* 3*/{}, {}, {},
    /* 4*/{}, {{"SHADOW_SOFT_ESM", ""}}, {},
    /* 5*/{}, {}, {},
    /* 6*/{}, {{"SSAO_RANGE", "0.4"}, {"SSAO_THRESHOLD", "1.0"}, {"SAMPLE_NUM", "16"}, {"SAMPLE_BIAS", "0.05"}}, {},
    /* 7*/{}, {}, {},
    /* 8*/{{"MAX_SEARCH_STEPS", "32"}}, {{"MAX_SEARCH_STEPS", "32"}}, {},
    /* 9*/{}, {}, {},
    /*10*/{}, {}, {}
};
const std::string _glsl_version = "#version 430 core\n";

const float quadVertices[] = {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};
uint _screen_vao, _screen_vbo;
void checkScreenVAO() {
    static bool is_screen_vao_initialized = false;
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

const float _envMapVertices[] = {
    // -z       
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    // -x
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    // +x
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     // +z
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    // +y
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    // -y
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};
uint _envmap_vao, _envmap_vbo;
glm::mat4 _capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 _capture_views[] =
{
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};
void checkEnvmapVAO() {
    static bool is_envmap_vao_initialized = false;
    if (!is_envmap_vao_initialized) {
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
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
}

struct TimeRecord {
    float time = 0.0f, time_last = 0.0f, time_current = 0.0f;
    void start() {
        time_current = glfwGetTime();
    }
    void stop() {
        time_last = time_current;
        time_current = glfwGetTime();
        time += time_current - time_last;
    }
    void clear() {
        time = time_last = time_current = 0.0f;
    }
    float getTime(bool is_clear = true) {
        float rtime = time;
        if (is_clear) clear();
        return rtime;
    }
};

GLuint _atomic_buf[2], _atomic_counter_data[8] = { 0 };
bool is_atomic_buf_vao_initialized = false;
struct Record {
    uint triangles;
    uint draw_calls;
    uint texture_samples;
    uint samples[8];
    GLuint* data;
    uint id = 0;
    void clear(bool clear_all = true) {
        triangles = draw_calls = texture_samples = 0;
        if (clear_all) {
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomic_buf[0]);
            glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, 8 * sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, _atomic_counter_data);
        }
    }
    void init() {
        if (!is_atomic_buf_vao_initialized) {
            glGenBuffers(2, _atomic_buf);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomic_buf[0]);
            glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8 * sizeof(GLuint), NULL, GL_DYNAMIC_COPY);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, _atomic_buf[0]);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomic_buf[1]);
            glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8 * sizeof(GLuint), NULL, GL_DYNAMIC_COPY);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, _atomic_buf[1]);
            is_atomic_buf_vao_initialized = true;
        }
        id = 0;
        clear();
    }
    void get() {
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomic_buf[1]);
        data = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8 * sizeof(GLuint), GL_MAP_READ_BIT);
        if (data != NULL) {
            samples[0] = data[0];
            samples[1] = data[1];
            samples[2] = data[2];
            samples[3] = data[3];
            samples[4] = data[4];
            samples[5] = data[5];
            samples[6] = data[6];
            samples[7] = data[7];
            texture_samples = samples[7] * 128 + samples[6] * 64 + samples[5] * 32 + samples[4] * 16 + samples[3] * 8 + samples[2] * 4 + samples[1] * 2 + samples[0];
        }
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        glCopyNamedBufferSubData(_atomic_buf[0], _atomic_buf[1], 0, 0, 8 * sizeof(GLuint));
    }
}frame_record;