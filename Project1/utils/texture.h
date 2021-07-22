#pragma once

#include "defines.h"
#include "DDSReader.h"

#include "tex/SearchTex.h"
#include "tex/AreaTex.h"

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
        Texture* tex = new Texture;
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tex->id);
        tex->target = GL_TEXTURE_CUBE_MAP;
        tex->width = width; tex->height = height;
        for (GLuint i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat,
                width, height, 0, format, type, NULL);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return tex;
    }
    static Texture* createFromFile(
        const char* path,
        const std::string& directory = "",
        bool genMipmap = true,
        bool gammaCorrection = false,
        bool filp_vertically_on_load = true
    ) {
        std::string p = std::string(path);
        std::string filename = directory == "" ? p : directory + '/' + p;

        std::string extname = p.substr(p.rfind('.', p.size()) + 1);
        if (extname == "dds") {
            TexProps tp = load_dds_textures(filename.c_str(), genMipmap, gammaCorrection);
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
            GLenum format, internalformat;
            internalformat = nrComponents == 1 ? GL_RED : nrComponents == 3 ? GL_RGB : GL_RGBA;
            format = gammaCorrection ? (nrComponents == 3 ? GL_SRGB : GL_SRGB_ALPHA) : internalformat;
            tex->format = format;

            glGenTextures(1, &tex->id);
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, internalformat, GL_UNSIGNED_BYTE, data);
            tex->width = width; tex->height = height;
            tex->target = GL_TEXTURE_2D;

            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _max_anisotropy);
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
        tex->width = width; tex->height = height;
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return tex;
    }
    static Texture* createHDRMapFromFile(
        std::string path,
        bool filp_vertically_on_load = true
    ) {
        Texture* tex = new Texture;
        glGenTextures(1, &tex->id);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        tex->target = GL_TEXTURE_2D;

        stbi_set_flip_vertically_on_load(filp_vertically_on_load);
        int width, height, nrComponents;
        float* data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
        tex->width = width; tex->height = height;
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load HDR image." << std::endl;
        }
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
        glGenerateMipmap(target);
    }
};

class TextureManager {
public:
    std::vector<Texture*> textures;

    TextureManager() {}
    void init() {
        uint tex_amount = _texture_paths.size();
        textures.resize(tex_amount);
        /*for (uint i = 0; i < tex_amount; i++) {
            textures[i] = Texture::createFromFile(_texture_paths[i], "", false, false, true);
        }*/
        textures[TID_BRDFLUT] = Texture::createFromFile(_texture_paths[TID_BRDFLUT], "", false, false, true);
        textures[TID_SMAA_SEARCHTEX] = Texture::createFromArray(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes, GL_NEAREST);
        textures[TID_SMAA_AREATEX] = Texture::createFromArray(AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes, GL_LINEAR);
        //textures[TID_SMAA_SEARCHTEX] = Texture::createFromFile(_texture_paths[TID_SMAA_SEARCHTEX], "", false, false, true);
        //textures[TID_SMAA_AREATEX] = Texture::createFromFile(_texture_paths[TID_SMAA_AREATEX], "", false, false, true);
    }
    Texture* getTex(uint index) {
        return textures[index];
    }
};
// All preloaded textures are accessible with TextureManager
TextureManager tManager;