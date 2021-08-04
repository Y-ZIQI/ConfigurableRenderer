#pragma once
#include "defines.h"
#include "constants.h"

// Need initialized variables
GLfloat _max_anisotropy;
uint _screen_vao, _screen_vbo;
uint _envmap_vao, _envmap_vbo;
GLuint _atomic_buf[2];
char _sprintf_tmp[64];
// Check error
#define CHECKERROR _checkError(__FILE__, __LINE__);
void _checkError(std::string file, uint line) {
    auto a = glGetError();
    if (a)
        std::cout << "Error " << a << " in " << file << ", line " << line << std::endl;
}
// Record Time
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
// Record render statistics
struct Record {
    bool initialized = false;
    uint triangles;
    uint draw_calls;
    uint texture_samples;
    GLuint data[8];

    void clear(bool clear_all = true) {
        triangles = draw_calls = 0;
        if (clear_all) {
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomic_buf[0]);
            glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, 8 * sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, _atomic_counter_clear_data);
        }
    }
    void init() {
        if (!initialized) {
            glGenBuffers(2, _atomic_buf);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomic_buf[0]);
            glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8 * sizeof(GLuint), NULL, GL_DYNAMIC_COPY);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, _atomic_buf[0]);
            glBindBuffer(GL_COPY_WRITE_BUFFER, _atomic_buf[1]);
            glBufferStorage(GL_COPY_WRITE_BUFFER, 8 * sizeof(GLuint), NULL, GL_MAP_READ_BIT);
            initialized = true;
        }
        clear();
    }
    void copy() {
        glCopyNamedBufferSubData(_atomic_buf[0], _atomic_buf[1], 0, 0, 8 * sizeof(GLuint));
    }
    void get() {
        glGetNamedBufferSubData(_atomic_buf[1], 0, 8 * sizeof(GLuint), data);
        texture_samples = data[7] * 128 + data[6] * 64 + data[5] * 32 + data[4] * 16 + data[3] * 8 + data[2] * 4 + data[1] * 2 + data[0];
    }
};
Record frame_record;

char* const getStrFormat(char const* const format, ...) {
    va_list ap;
    va_start(ap, format);
    int n = _vsprintf_l(_sprintf_tmp, format, NULL, ap);
    va_end(ap);
    return (char* const)_sprintf_tmp;
}

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
// Render a Screen Rectangle
void renderScreen(bool depth_test = false) {
    checkScreenVAO();
    if (!depth_test) glDisable(GL_DEPTH_TEST);
    else glEnable(GL_DEPTH_TEST);
    glBindVertexArray(_screen_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    frame_record.triangles += 2;
    frame_record.draw_calls += 1;
}
// Render a Skybox Cube
void renderCube(bool depth_test = true) {
    checkEnvmapVAO();
    if (depth_test) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    glBindVertexArray(_envmap_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    frame_record.triangles += 12;
    frame_record.draw_calls += 1;
}