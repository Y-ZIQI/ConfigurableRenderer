#pragma once
#include <windows.h>
#include "defines.h"

// Minimum and maximum macros
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))


//PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB = NULL;


#pragma region DDS

#define DDPF_ALPHAPIXELS    0x000001
#define DDPF_ALPHA            0x000002
#define DDPF_FOURCC            0x000004
#define DDPF_RGB            0x000040
#define DDPF_YUV            0x000200
#define DDPF_LUMINANCE        0x020000

#define D3DFMT_DXT1    (('D'<<0)|('X'<<8)|('T'<<16)|('1'<<24))
#define D3DFMT_DXT3    (('D'<<0)|('X'<<8)|('T'<<16)|('3'<<24))
#define D3DFMT_DXT5    (('D'<<0)|('X'<<8)|('T'<<16)|('5'<<24))
#define D3DFMT_ATI2    (('A'<<0)|('T'<<8)|('I'<<16)|('2'<<24))

typedef struct
{
    DWORD    dwSize;
    DWORD    dwFlags;
    DWORD    dwFourCC;
    DWORD    dwRGBBitCount;
    DWORD    dwRBitMask;
    DWORD    dwGBitMask;
    DWORD    dwBBitMask;
    DWORD    dwABitMask;
} DDS_PIXELFORMAT;

#define DDSD_CAPS            0x000001
#define DDSD_HEIGHT            0x000002
#define DDSD_WIDTH            0x000004
#define DDSD_PITCH            0x000008
#define DDSD_PIXELFORMAT    0x001000
#define DDSD_MIPMAPCOUNT    0x020000
#define DDSD_LINEARSIZE        0x080000
#define DDSD_DEPTH            0x800000

typedef struct
{
    DWORD            dwSize;
    DWORD            dwFlags;
    DWORD            dwHeight;
    DWORD            dwWidth;
    DWORD            dwPitchOrLinearSize;
    DWORD            dwDepth;
    DWORD            dwMipMapCount;
    DWORD            dwReserved1[11];
    DDS_PIXELFORMAT    ddspf;
    DWORD            dwCaps;
    DWORD            dwCaps2;
    DWORD            dwCaps3;
    DWORD            dwCaps4;
    DWORD            dwReserved2;
} DDS_HEADER;

typedef struct
{
    DWORD        dwMagic;
    DDS_HEADER    Header;
} DDS_FILEHEADER;

// For a compressed texture, the size of each mipmap level image is typically one-fourth the size of the previous, with a minimum of 8 (DXT1) or 16 (DXT2-5) bytes (for 
// square textures). Use the following formula to calculate the size of each level for a non-square texture:
#define SIZE_OF_DXT1(width, height)    ( __max(1, ( (width + 3) >> 2 ) ) * __max(1, ( (height + 3) >> 2 ) ) * 8 )
#define SIZE_OF_DXT2(width, height)    ( __max(1, ( (width + 3) >> 2 ) ) * __max(1, ( (height + 3) >> 2 ) ) * 16 )

#pragma endregion

struct TexProps {
    GLuint id;
    GLenum format;
    uint width, height;
};

bool gl_load_dds(TexProps &props, GLvoid *pBuffer, const char* path, bool genMipmap, bool gammaCorrection)
{
    DDS_FILEHEADER    *header;
    DWORD            compressFormat;
    GLuint            texnum;
    GLvoid            *data;
    GLsizei            imageSize;

    header = (DDS_FILEHEADER *)pBuffer;

    if (header->dwMagic != 0x20534444) {
        printf("bad dds file: %s\n", path);
        return false;
    }

    if (header->Header.dwSize != 124) {
        printf("bad header size: %s\n", path);
        return false;
    }

    if (!(header->Header.dwFlags & DDSD_LINEARSIZE)) {
        printf("bad file type: %s\n", path);
        return false;
    }

    if (!(header->Header.ddspf.dwFlags & DDPF_FOURCC)) {
        printf("bad pixel format: %s\n", path);
        return false;
    }

    compressFormat = header->Header.ddspf.dwFourCC;

    if (compressFormat != D3DFMT_DXT1 &&
        compressFormat != D3DFMT_DXT3 &&
        compressFormat != D3DFMT_DXT5 &&
        compressFormat != D3DFMT_ATI2) {
            printf("bad compress format: %s\n", path);
            return false;
    }

    data = (GLvoid *)(header + 1);    // header data skipped

    glGenTextures(1, &texnum);
    glBindTexture(GL_TEXTURE_2D, texnum);
    props.width = header->Header.dwWidth;
    props.height = header->Header.dwHeight;

    GLenum format;
    switch (compressFormat)
    {
    case D3DFMT_DXT1:
        props.format = gammaCorrection ? GL_SRGB : GL_RGB;
        format = gammaCorrection ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        imageSize = SIZE_OF_DXT1(header->Header.dwWidth, header->Header.dwHeight);
        glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, format, header->Header.dwWidth, header->Header.dwHeight, 0, imageSize, data);
        break;
    case D3DFMT_DXT3:
        props.format = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        format = gammaCorrection ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        imageSize = SIZE_OF_DXT2(header->Header.dwWidth, header->Header.dwHeight);
        glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, format, header->Header.dwWidth, header->Header.dwHeight, 0, imageSize, data);
        break;
    case D3DFMT_DXT5:
        props.format = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        format = gammaCorrection ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        imageSize = SIZE_OF_DXT2(header->Header.dwWidth, header->Header.dwHeight);
        glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, format, header->Header.dwWidth, header->Header.dwHeight, 0, imageSize, data);
        break;
    case D3DFMT_ATI2:
        props.format = GL_RG;
        format = GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
        imageSize = SIZE_OF_DXT2(header->Header.dwWidth, header->Header.dwHeight);
        glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, format, header->Header.dwWidth, header->Header.dwHeight, 0, imageSize, data);
        break;
    }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _max_anisotropy);
    if (genMipmap) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    props.id = texnum;
    return true;
}

TexProps load_dds_textures(const char* path, bool genMipmap, bool gammaCorrection)
{
    FILE    *fp;
    int     size;
    void    *data;

    TexProps t{ 0, 0, 0, 0 };
    fp = fopen(path, "rb");
    if (!fp) {
        return t;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data = malloc(size);
    if (!data) {
        fclose(fp);
        return t;
    }

    if (fread(data, size, 1, fp) != 1) {
        free(data);
        fclose(fp);
        return t;
    }

    fclose(fp);

    // Load DDS to GL texture
    bool success = gl_load_dds(t, data, path, genMipmap, gammaCorrection);
    free(data);

    if (success)
        return t;
    else
        return { 0, 0, 0, 0 };
}