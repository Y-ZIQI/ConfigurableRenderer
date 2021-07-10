#pragma once

#include "texture.h"

class FrameBuffer {
public:
    uint id = 0;
    struct Attachment
    {
        Texture* texture;
        int mipLevel = 0;
        int arraySize = 1;
        int firstArraySlice = 0;
    };
    struct RenderBuffer
    {
        uint id = 0;
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
        else
            attachs = 1;
        colorAttachs.resize(MAX_TARGETS);
    };
    void attachColorTarget(Texture* tex, uint index) {
        if (index >= MAX_TARGETS) return;
        if (index >= attachs) attachs = index + 1;
        colorAttachs[index].texture = tex;
        use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, tex->id, 0);
        unbind();
    };
    void attachCubeTarget(Texture* tex, uint index) {
        if (index >= MAX_TARGETS) return;
        if (index >= attachs) attachs = index + 1;
        colorAttachs[index].texture = tex;
        use();
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, tex->id, 0);
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
    void attachCubeDepthTarget(Texture* tex) {
        depthAttach.texture = tex;
        use();
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->id, 0);
        unbind();
    }
    void attachDepthTarget(Texture* tex) {
        depthAttach.texture = tex;
        use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->id, 0);
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
    void prepare(int flag = ALL_TARGETS) {
        use();
        if (id != 0) {
            if(flag == ALL_TARGETS)
                switch (attachs) {
                case 0:
                    glDrawBuffer(GL_NONE); break;
                case 1:
                    glDrawBuffer(_color_attachments[0]); break;
                default:
                    glDrawBuffers(attachs, _color_attachments);
                }
            else
                glDrawBuffer(_color_attachments[flag]);
        }
    }
    /**
    * Default means clear all
    */
    void clear(int flag = ALL_TARGETS, const GLfloat* color = _clear_color) {
        if (flag == ALL_TARGETS) {
            use();
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            unbind();
        }
        else if (flag == -1) {
            use();
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            unbind();
        }
        else if(flag >= 0){
            glClearBufferfv(id, GL_COLOR_ATTACHMENT0 + flag, color);
        }
    }
};