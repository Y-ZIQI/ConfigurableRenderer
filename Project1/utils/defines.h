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
const GLuint _color_attachments[MAX_TARGETS] = {
	GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7
};
const GLfloat _clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
const GLfloat _clear_color_1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

#define SID_FORWARD 0
#define SID_DEFERRED_BASE 1
#define SID_DEFERRED_SHADING 2
#define SID_FORWARD_ENVMAP 3
#define SID_DEFERRED_ENVMAP 4
#define SID_UPSAMPLING 5
#define SID_SHADOWMAP 6
#define SID_CUBEMAP_RENDER 7
#define SID_SSAO 8
const std::vector<const char*> _shader_paths
{
    "shaders/main/mainvs.vs", "shaders/main/mainfs.fs", nullptr,
    "shaders/deferred/basePass.vs", "shaders/deferred/basePass.fs", nullptr,
    "shaders/deferred/shadingPass.vs", "shaders/deferred/shadingPass.fs", nullptr,
    "shaders/envmap/forward.vs", "shaders/envmap/forward.fs", nullptr,
    "shaders/envmap/deferred.vs", "shaders/envmap/deferred.fs", nullptr,
    "shaders/upsampling/main.vs", "shaders/upsampling/main.fs", nullptr,
    "shaders/shadow/shadow.vs", "shaders/shadow/shadow.fs", nullptr,
    "shaders/omnidirectional/main.vs", "shaders/omnidirectional/main.fs", "shaders/omnidirectional/main.gs",
    "shaders/ssao/ssao.vs", "shaders/ssao/ssao.fs", nullptr
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