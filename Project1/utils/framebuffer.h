#pragma once

#include "texture.h"

class FrameBuffer {
public:
    uint id = UINT_MAX;
    struct Attachment
    {
        Texture* texture = nullptr;
        int mipLevel = 0;
        int arraySize = 1;
        int firstArraySlice = 0;
    };
    struct RenderBuffer
    {
        uint id = UINT_MAX;
        ~RenderBuffer() {
            if (glIsRenderbuffer(id)) glDeleteRenderbuffers(1, &id);
        };
    };
    std::vector<Attachment> colorAttachs;
    Attachment depthAttach;
    RenderBuffer depthRB;
    RenderBuffer depthStencil;

    uint attachs;

    FrameBuffer(bool isDefaultFrameBuffer = false) {
        if (!isDefaultFrameBuffer) {
            glGenFramebuffers(1, &id);
            attachs = 0;
        }
        else {
            id = 0;
            attachs = 1;
        }
        colorAttachs.resize(MAX_TARGETS);
    };
    ~FrameBuffer() {
        if (id != 0 && glIsFramebuffer(id)) glDeleteFramebuffers(1, &id);
    }
    void attachColorTarget(Texture* tex, uint index, GLenum texTarget = GL_TEXTURE_2D, GLint miplevel = 0) {
        if (index >= MAX_TARGETS) return;
        if (index >= attachs) attachs = index + 1;
        colorAttachs[index].texture = tex;
        use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, texTarget, tex->id, miplevel);
        unbind();
    };
    void attachCubeTarget(Texture* tex, uint index, GLint miplevel = 0) {
        if (index >= MAX_TARGETS) return;
        if (index >= attachs) attachs = index + 1;
        colorAttachs[index].texture = tex;
        use();
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, tex->id, miplevel);
        unbind();
    }
    void addDepthRenderbuffer(uint width, uint height) {
        use();
        glGenRenderbuffers(1, &depthRB.id);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRB.id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRB.id);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        unbind();
    }
    void addDepthStencil(uint width, uint height) {
        use();
        glGenRenderbuffers(1, &depthStencil.id);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencil.id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil.id);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        unbind();
    }
    void attachCubeDepthTarget(Texture* tex, GLint miplevel = 0) {
        depthAttach.texture = tex;
        use();
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->id, miplevel);
        unbind();
    }
    void attachDepthTarget(Texture* tex, GLenum texTarget = GL_TEXTURE_2D, GLint miplevel = 0) {
        depthAttach.texture = tex;
        use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texTarget, tex->id, miplevel);
        unbind();
    }
    void use() {
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    }
    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    void setParami(GLenum pname, GLint param) {
        use();
        glFramebufferParameteri(GL_FRAMEBUFFER, pname, param);
        unbind();
    }
    bool checkStatus() {
        use();
        auto a = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        unbind();
        return a == GL_FRAMEBUFFER_COMPLETE;
    }
    // Prepare all targets
    void _prepare() {
        switch (attachs) {
        case 0: glDrawBuffer(GL_NONE); break;
        case 1: glDrawBuffer(_color_attachments[0]); break;
        default: glDrawBuffers(attachs, _color_attachments);
        }
    }
    // Prepare all targets or a particular target
    void prepare(int flag = ALL_TARGETS) {
        use();
        if (id != 0) {
            if (flag == ALL_TARGETS) _prepare();
            else glDrawBuffer(_color_attachments[flag]);
        }
    }
    // Clear all targets or depthstencil or a particular target
    void clear(int flag = ALL_TARGETS, const GLfloat* color = _clear_color) {
        use();
        if (flag == ALL_TARGETS) {
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
        else if (flag == DEPTH_TARGETS) {
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
        else if(flag >= 0){
            glClearBufferfv(id, GL_COLOR_ATTACHMENT0 + flag, color);
        }
    }
};