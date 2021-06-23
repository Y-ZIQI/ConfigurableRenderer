#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nanogui/nanogui.h>

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
#define SID_FORWARD 0
#define SID_DEFERRED_BASE 1
#define SID_DEFERRED_SHADING 2
#define SID_FORWARD_ENVMAP 3
#define SID_DEFERRED_ENVMAP 4
#define SID_UPSAMPLING 5
#define SID_SHADOWMAP 6
#define SID_CUBEMAP_RENDER 7
#define SID_SSAO 8
#define SID_SMAA_EDGEPASS 9
#define SID_SMAA_BLENDPASS 10
#define SID_SMAA_NEIGHBORPASS 11
const std::vector<const char*> _shader_paths
{
    /* 0*/"shaders/main/mainvs.vs", "shaders/main/mainfs.fs", nullptr,
    /* 1*/"shaders/deferred/basePass.vs", "shaders/deferred/basePass.fs", nullptr,
    /* 2*/"shaders/deferred/shadingPass.vs", "shaders/deferred/shadingPass.fs", nullptr,
    /* 3*/"shaders/envmap/forward.vs", "shaders/envmap/forward.fs", nullptr,
    /* 4*/"shaders/envmap/deferred.vs", "shaders/envmap/deferred.fs", nullptr,
    /* 5*/"shaders/upsampling/main.vs", "shaders/upsampling/main.fs", nullptr,
    /* 6*/"shaders/shadow/shadow.vs", "shaders/shadow/shadow.fs", nullptr,
    /* 7*/"shaders/omnidirectional/main.vs", "shaders/omnidirectional/main.fs", "shaders/omnidirectional/main.gs",
    /* 8*/"shaders/ssao/ssao.vs", "shaders/ssao/ssao.fs", nullptr,
    /* 9*/"shaders/smaa/edgeDetection.vs", "shaders/smaa/edgeDetection.fs", nullptr,
    /*10*/"shaders/smaa/blendCalculation.vs", "shaders/smaa/blendCalculation.fs", nullptr,
    /*11*/"shaders/smaa/neighborBlending.vs", "shaders/smaa/neighborBlending.fs", nullptr
};
const std::vector<std::initializer_list<std::pair<const std::string, std::string>>> _shader_defs
{
    /* 0*/{}, {}, {},
    /* 1*/{}, {}, {},
    /* 2*/{}, {}, {},
    /* 3*/{}, {}, {},
    /* 4*/{}, {}, {},
    /* 5*/{}, {}, {},
    /* 6*/{}, {{"SHADOW_SOFT_ESM", ""}}, {},
    /* 7*/{}, {}, {},
    /* 8*/{}, {{"SSAO_RANGE", "0.4"}, {"SSAO_THRESHOLD", "1.0"}, {"SAMPLE_NUM", "16"}, {"SAMPLE_BIAS", "0.05"}}, {},
    /* 9*/{}, {}, {},
    /*10*/{{"MAX_SEARCH_STEPS", "32"}}, {{"MAX_SEARCH_STEPS", "32"}}, {},
    /*11*/{}, {}, {}
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
bool is_screen_vao_initialized = false;

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
bool is_envmap_vao_initialized = false;