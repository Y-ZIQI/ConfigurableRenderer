#pragma once

#include "shader.h"
#include "texture.h"

class RandomHemisphereSample {
public:
    float radius;
    uint size;
    std::default_random_engine generator;
    std::uniform_real_distribution<GLfloat> randomFloats;
    std::vector<glm::vec3> samples;

    RandomHemisphereSample() {};
    void init(float radius, uint size) {
        this->radius = radius;
        this->size = size;

        samples.resize(size);
        randomFloats = std::uniform_real_distribution<GLfloat>(0.0, 1.0);
        for (uint i = 0; i < size; i++) {
            glm::vec3 sample(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator)
            );
            sample = glm::normalize(sample);
            sample *= randomFloats(generator) * radius;
            samples[i] = sample;
        }
    }
    void setUniforms(Shader* shader, uint amount = 0) {
        char tmp[20];
        amount = (amount <= 0 || amount >= size) ? size : amount;
        for (uint i = 0; i < amount; i++) {
            sprintf(tmp, "samples[%d]", i);
            shader->setVec3(tmp, samples[i]);
        }
    }
};

class PoissonDiskSample {
public:
    /**
    * If sample is fixed, a 2-d poission disk will be given to the shader;
    * if not fixed, a ring array(radius = 1) and a radius array will be given to shader,
    * and you will need to combine ring and radius.
    * e.g. poissonRing[i] * poissonRadius[j]
    */
    bool fixed_sample;
    float radius, rings;
    uint size;
    std::default_random_engine generator;
    std::uniform_real_distribution<GLfloat> randomFloats;
    std::vector<glm::vec2> samples;
    std::vector<float> radiuses;

    PoissonDiskSample() {};
    void init(float radius, float rings, uint size, bool fixed_sample = true) {
        this->fixed_sample = fixed_sample;
        this->radius = radius;
        this->rings = rings;
        this->size = size;

        samples.resize(size);
        radiuses.resize(size);
        randomFloats = std::uniform_real_distribution<GLfloat>(0.0, 1.0);
        float angle_step = M_2PI * rings / float(size);
        float inv_samples = radius / float(size);
        float angle = randomFloats(generator) * M_2PI;
        float nradius = inv_samples;
        float radiusStep = nradius;
        if (fixed_sample) {
            for (uint i = 0; i < size; i++) {
                samples[i] = glm::vec2(cosf(angle), sinf(angle)) * powf(nradius, 0.75);
                nradius += radiusStep;
                angle += angle_step;
            }
        }
        else {
            for (uint i = 0; i < size; i++) {
                samples[i] = glm::vec2(cosf(angle), sinf(angle));
                radiuses[i] = powf(nradius, 0.75);
                nradius += radiusStep;
                angle += angle_step;
            }
        }
    }
    void setUniforms(Shader* shader, uint amount = 0) {
        char tmp[20];
        amount = (amount <= 0 || amount >= size) ? size : amount;
        if (fixed_sample) {
            for (uint i = 0; i < amount; i++) {
                sprintf(tmp, "samples[%d]", i);
                shader->setVec2(tmp, samples[i]);
            }
        }
        else {
            for (uint i = 0; i < amount; i++) {
                sprintf(tmp, "samples[%d]", i);
                shader->setVec2(tmp, samples[i]);
                sprintf(tmp, "radius[%d]", i);
                shader->setFloat(tmp, radiuses[i]);
            }
        }
    }
};

class HammersleySample {
public:
    uint size;
    std::vector<glm::vec2> samples;

    static glm::vec2 Hammersley(uint i, uint N) { // 0-1
        uint bits = (i << 16u) | (i >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        float rdi = float(bits) * 2.3283064365386963e-10;
        return { float(i) / float(N), rdi };
    }

    HammersleySample() {};
    void init(uint size) {
        this->size = size;

        samples.resize(size);
        for (uint i = 0; i < size; i++) {
            samples[i] = Hammersley(i, size);
        }
    }
    void setUniforms(Shader* shader, uint amount = 0) {
        char tmp[20];
        amount = (amount <= 0 || amount >= size) ? size : amount;
        for (uint i = 0; i < amount; i++) {
            sprintf(tmp, "samples[%d]", i);
            shader->setVec2(tmp, samples[i]);
        }
    }
};