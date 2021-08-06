#pragma once
#include "includes.h"

// For Framebuffers
const GLuint _color_attachments[] = {
    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7,
    GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
    GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15
};
// For Clear
const GLfloat _clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
const GLfloat _clear_color_1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat _clear_color_evsm[4] = { 3.4028e38f, 3.4028e38f, 0.0f, 0.0f };
const GLuint _atomic_counter_clear_data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
// For Shaders
const std::vector<const char*> _shader_paths
{
    /* 0*/"shaders/deferred/basePass.vs", "shaders/deferred/basePass.fs", nullptr,
    /* 1*/"shaders/deferred/preprocess.vs", "shaders/deferred/preprocess.fs", nullptr,
    /* 2*/"shaders/deferred/shadingPass.vs", "shaders/deferred/shadingPass.fs", nullptr,
    /* 3*/"shaders/deferred/filter.vs", "shaders/deferred/filter.fs", nullptr,
    /* 4*/"shaders/envmap/deferred.vs", "shaders/envmap/deferred.fs", nullptr,
    /* 5*/"shaders/upsampling/main.vs", "shaders/upsampling/main.fs", nullptr,
    /* 6*/"shaders/shadow/shadow.vs", "shaders/shadow/shadow.fs", nullptr,
    /* 7*/"shaders/shadow/filter.vs", "shaders/shadow/filter.fs", nullptr,
    /* 8*/"shaders/omnidirectional/main.vs", "shaders/omnidirectional/main.fs", "shaders/omnidirectional/main.gs",
    /* 9*/"shaders/smaa/edgeDetection.vs", "shaders/smaa/edgeDetection.fs", nullptr,
    /*10*/"shaders/smaa/blendCalculation.vs", "shaders/smaa/blendCalculation.fs", nullptr,
    /*11*/"shaders/smaa/neighborBlending.vs", "shaders/smaa/neighborBlending.fs", nullptr,
    /*12*/"shaders/ssr/rayTrace.vs", "shaders/ssr/rayTrace.fs", nullptr,
    /*13*/"shaders/ssr/reuse.vs", "shaders/ssr/reuse.fs", nullptr,
    /*14*/"shaders/ssr/blur.vs", "shaders/ssr/blur.fs", nullptr,
    /*15*/"shaders/ibl/prefilter.vs", "shaders/ibl/prefilter.fs", nullptr,
    /*16*/"shaders/post/blur.vs", "shaders/post/blur.fs", nullptr,
    /*17*/"shaders/post/join.vs", "shaders/post/join.fs", nullptr,
    /*18*/"shaders/shadow/omnishadow.vs", "shaders/shadow/omnishadow.fs", "shaders/shadow/omnishadow.gs",
    /*19*/"shaders/shadow/omnifilter.vs", "shaders/shadow/omnifilter.fs", "shaders/shadow/omnifilter.gs",
    /*20*/"shaders/ssr/tiling.vs", "shaders/ssr/tiling.fs", nullptr
};
const std::string _glsl_version = "#version 430 core\n";
// For preloaded Textures
const std::vector<const char*> _texture_paths
{
    /* 0*/"resources/textures/ibl_brdf_lut.png",
    /* 1*/"resources/textures/SearchTex.dds",
    /* 2*/"resources/textures/AreaTexDX10.dds"
};
// Screen quadrangle
const float quadVertices[] = {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};
// Skybox cube
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
// Proj/View Matrix in cube rendering
const glm::mat4 _capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
const glm::mat4 _capture_views[] = {
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};