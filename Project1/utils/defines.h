#pragma once
#include "includes.h"

typedef unsigned int uint;
typedef unsigned char uchar;

#define UINT_MAX 0xFFFFFFFF
#define FLOAT_MAX 3.4028e38f
// Math Defines
#define M_PI                3.1415926536f  // pi
#define M_2PI               6.2831853072f  // 2pi
#define M_1_PI              0.3183098862f // 1/pi
// For Gbuffer
#define GBUFFER_TARGETS         8
#define MAX_DIRECTIONAL_LIGHT   4
#define MAX_POINT_LIGHT         15
#define MAX_RADIOACTIVE_LIGHT   15
#define MAX_IBL_LIGHT           10
// For Framebuffer
#define MAX_TARGETS     8
#define ALL_TARGETS     -2
#define DEPTH_TARGETS   -1
// Shader type
#define VSHADER 0
#define FSHADER 1
#define GSHADER 2
// Shaders
#define SID_DEFERRED_BASE           0
#define SID_DEFERRED_ENVMAP         4
#define SID_DEFERRED_PREPROCESS     1
#define SID_SSAO_FILTER             3
#define SID_DEFERRED_SHADING        2
#define SID_SSR_TILING              20
#define SID_SSR_RAYTRACE            12
#define SID_SSR_RESOLVE             13
#define SID_SSR_BLUR                14
#define SID_BLOOM_BLUR              16
#define SID_JOIN_EFFECTS            17
#define SID_SMAA_EDGEPASS           9
#define SID_SMAA_BLENDPASS          10
#define SID_SMAA_NEIGHBORPASS       11
#define SID_UPSAMPLING              5
#define SID_SHADOWMAP               6
#define SID_SHADOWMAP_FILTER        7
#define SID_OMNISHADOWMAP           18
#define SID_OMNISHADOWMAP_FILTER    19
#define SID_CUBEMAP_RENDER          8
#define SID_IBL_PREFILTER           15
// Preloaded Textures
#define TID_BRDFLUT                 0
#define TID_SMAA_SEARCHTEX          1
#define TID_SMAA_AREATEX            2