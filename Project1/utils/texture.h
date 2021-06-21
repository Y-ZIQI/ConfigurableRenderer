#pragma once

#include "defines.h"
#include "DDSReader.h"

class Texture {
public:
    uint id;
    std::string type;
    std::string path;
    uint width, height;
    GLenum target, format;

    Texture() {};
    static Texture* create(
        uint width,
        uint height,
        GLenum internalFormat,
        GLenum format,
        GLenum type,
        GLint filter = GL_NEAREST,
        GLint wrap = GL_CLAMP_TO_EDGE
    ) {
        Texture* tex = new Texture;
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        tex->target = GL_TEXTURE_2D;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
        tex->width = width; tex->height = height;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }
    static Texture* createFromArray(
        uint width,
        uint height,
        GLenum internalFormat,
        GLenum format,
        GLenum type,
        const void* data,
        GLint filter = GL_NEAREST,
        GLint wrap = GL_CLAMP_TO_EDGE
    ) {
        Texture* tex = new Texture;
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        tex->target = GL_TEXTURE_2D;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
        tex->width = width; tex->height = height;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }
    static Texture* createCubeMap(
        uint width,
        uint height,
        GLenum internalFormat,
        GLenum format,
        GLenum type,
        GLint filter = GL_NEAREST
    ) {
        auto b = glGetError();
        Texture* tex = new Texture;
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tex->id);
        tex->target = GL_TEXTURE_CUBE_MAP;
        tex->width = width; tex->height = height;
        b = glGetError();
        for (GLuint i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat,
                width, height, 0, format, type, NULL);
        b = glGetError();
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filter);
        b = glGetError();
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filter);
        b = glGetError();
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        b = glGetError();
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        b = glGetError();
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        b = glGetError();
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return tex;
    }
    static Texture* createFromFile(
        const char* path,
        const std::string& directory = "",
        bool genMipmap = true,
        bool filp_vertically_on_load = true
    ) {
        std::string p = std::string(path);
        std::string filename = directory + '/' + p;

        std::string extname = p.substr(p.rfind('.', p.size()) + 1);
        if (extname == "dds") {
            TexProps tp = load_dds_textures(filename.c_str(), genMipmap);
            if (tp.id == -1) return nullptr;
            Texture* tex = new Texture;
            tex->id = tp.id;
            tex->format = tp.format;
            tex->width = tp.width; tex->height = tp.height;
            tex->target = GL_TEXTURE_2D;
            return tex;
        }

        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(filp_vertically_on_load);
        uchar* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            Texture* tex = new Texture;
            GLenum format;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 3) format = GL_RGB;
            else if (nrComponents == 4) format = GL_RGBA;
            tex->format = format;

            glGenTextures(1, &tex->id);
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            tex->width = width; tex->height = height;
            tex->target = GL_TEXTURE_2D;
            if (genMipmap)
                glGenerateMipmap(GL_TEXTURE_2D);
            if (genMipmap)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            else
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            stbi_image_free(data);
            return tex;
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
            return nullptr;
        }
    }
    static Texture* createCubeMapFromFile(
        std::vector<std::string> paths,
        bool filp_vertically_on_load = true
    ) {
        Texture* tex = new Texture;
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tex->id);
        tex->target = GL_TEXTURE_CUBE_MAP;

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(filp_vertically_on_load);
        for (unsigned int i = 0; i < paths.size(); i++)
        {
            uchar* data = stbi_load(paths[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap texture failed to load at path: " << paths[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return tex;
    }
    void use() {
        glBindTexture(target, id);
    }
    void unbind() {
        glBindTexture(target, 0);
    }
    void setTexParami(GLenum pname, GLint param) {
        glTextureParameteri(id, pname, param);
    }
    void genMipmap() {
        use();
        glGenerateMipmap(GL_TEXTURE_2D);
        unbind();
    }
};
