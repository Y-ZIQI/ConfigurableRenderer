//++`shaders/shading/defines.glsl`

#ifndef MAX_SEARCH_STEPS
#define MAX_SEARCH_STEPS 32
#endif

out vec4 FragColor;

in vec2 TexCoords;
in vec2 PixCoords;
in vec4 offset[3];

uniform sampler2D edgeTex_point;
uniform sampler2D edgeTex_linear;
uniform sampler2D areaTex;
uniform sampler2D searchTex;
uniform float width;
uniform float height;
uniform vec4 subsampleIndices;

const vec4 pixel_size = vec4(1.0/width, 1.0/height, width, height);

//-----------------------------------------------------------------------------
// Non-Configurable Defines

#define SMAA_AREATEX_MAX_DISTANCE 16
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / vec2(160.0, 560.0))
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)
#define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0)
#define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0)
#define SMAA_CORNER_ROUNDING 25
#define SMAA_CORNER_ROUNDING_NORM (float(SMAA_CORNER_ROUNDING) / 100.0)
#define SMAA_MAX_SEARCH_STEPS_DIAG 16

float SearchLength(vec2 e, float offset){
    vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0);
    vec2 bias = SMAA_SEARCHTEX_SIZE * vec2(offset, 1.0);

    // Scale and bias to access texel centers:
    scale += vec2(-1.0,  1.0);
    bias  += vec2( 0.5, -0.5);

    // Convert from pixel coordinates to texcoords:
    // (We use SMAA_SEARCHTEX_PACKED_SIZE because the texture is cropped)
    scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
    bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;

    // Lookup the search texture:
    ATOMIC_COUNT_INCREMENT
    return texture(searchTex, fma(scale, e ,bias)).r;
}

float SearchXLeft(vec2 texcoord, float end){
    vec2 e = vec2(0.0, 1.0);
    while (texcoord.x > end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r < 0.01) { // Or is there a crossing edge that breaks the line?
        e = texture(edgeTex_linear, texcoord).rg;
        ATOMIC_COUNT_INCREMENT
        texcoord = fma(vec2(-2.0, 0.0), pixel_size.xy, texcoord);
    }

    float offset = fma((-255.0 / 127.0), SearchLength(e, 0.0), 3.25);
    return fma(pixel_size.x, offset, texcoord.x);
}

float SearchXRight(vec2 texcoord, float end){
    vec2 e = vec2(0.0, 1.0);
    while (texcoord.x < end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r < 0.01) { // Or is there a crossing edge that breaks the line?
        e = texture(edgeTex_linear, texcoord).rg;
        ATOMIC_COUNT_INCREMENT
        texcoord = fma(vec2(2.0, 0.0), pixel_size.xy, texcoord);
    }

    float offset = fma((-255.0 / 127.0), SearchLength(e, 0.5), 3.25);
    return fma(-pixel_size.x, offset, texcoord.x);
}

float SearchYUp(vec2 texcoord, float end){
    vec2 e = vec2(1.0, 0.0);
    while (texcoord.y < end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g < 0.01) { // Or is there a crossing edge that breaks the line?
        e = texture(edgeTex_linear, texcoord).rg;
        ATOMIC_COUNT_INCREMENT
        texcoord = fma(vec2(0.0, 2.0), pixel_size.xy, texcoord);
    }

    float offset = fma((-255.0 / 127.0), SearchLength(e.gr, 0.0), 3.25);
    return fma(-pixel_size.y, offset, texcoord.y);
}

float SearchYDown(vec2 texcoord, float end){
    vec2 e = vec2(1.0, 0.0);
    while (texcoord.y > end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g < 0.01) { // Or is there a crossing edge that breaks the line?
        e = texture(edgeTex_linear, texcoord).rg;
        ATOMIC_COUNT_INCREMENT
        texcoord = fma(vec2(0.0, -2.0), pixel_size.xy, texcoord);
    }

    float offset = fma((-255.0 / 127.0), SearchLength(e.gr, 0.5), 3.25);
    return fma(pixel_size.y, offset, texcoord.y);
}

vec2 SMAAArea(vec2 dist, float e1, float e2, float offset) {
    // Rounding prevents precision errors of bilinear filtering:
    vec2 texcoord = fma(vec2(SMAA_AREATEX_MAX_DISTANCE, SMAA_AREATEX_MAX_DISTANCE), round(4.0 * vec2(e1, e2)), dist);
    // We do a scale and bias for mapping to texel space:
    texcoord = fma(SMAA_AREATEX_PIXEL_SIZE, texcoord, 0.5 * SMAA_AREATEX_PIXEL_SIZE);
    // Move to proper place, according to the subpixel offset:
    texcoord.y = fma(SMAA_AREATEX_SUBTEX_SIZE, offset, texcoord.y);

    ATOMIC_COUNT_INCREMENT
    return texture(areaTex, texcoord).rg;
}

//-----------------------------------------------------------------------------
// Corner Detection Functions

void SMAADetectHorizontalCornerPattern(inout vec2 weights, vec4 texcoord, vec2 d) {
    vec2 leftRight = step(d.xy, d.yx);
    vec2 rounding = (1.0 - SMAA_CORNER_ROUNDING_NORM) * leftRight;

    rounding /= leftRight.x + leftRight.y; // Reduce blending for pixels in the center of a line.

    vec2 factor = vec2(1.0, 1.0);
    factor.x -= rounding.x * textureOffset(edgeTex_linear, texcoord.xy, ivec2(0,  -1)).r;
    ATOMIC_COUNT_INCREMENT
    factor.x -= rounding.y * textureOffset(edgeTex_linear, texcoord.zw, ivec2(1,  -1)).r;
    ATOMIC_COUNT_INCREMENT
    factor.y -= rounding.x * textureOffset(edgeTex_linear, texcoord.xy, ivec2(0, 2)).r;
    ATOMIC_COUNT_INCREMENT
    factor.y -= rounding.y * textureOffset(edgeTex_linear, texcoord.zw, ivec2(1, 2)).r;
    ATOMIC_COUNT_INCREMENT

    weights *= clamp(factor, 0.0, 1.0);
}

void SMAADetectVerticalCornerPattern(inout vec2 weights, vec4 texcoord, vec2 d) {
    vec2 leftRight = step(d.xy, d.yx);
    vec2 rounding = (1.0 - SMAA_CORNER_ROUNDING_NORM) * leftRight;

    rounding /= leftRight.x + leftRight.y;

    vec2 factor = vec2(1.0, 1.0);
    factor.x -= rounding.x * textureOffset(edgeTex_linear, texcoord.xy, ivec2( 1, 0)).g;
    ATOMIC_COUNT_INCREMENT
    factor.x -= rounding.y * textureOffset(edgeTex_linear, texcoord.zw, ivec2( 1, -1)).g;
    ATOMIC_COUNT_INCREMENT
    factor.y -= rounding.x * textureOffset(edgeTex_linear, texcoord.xy, ivec2(-2, 0)).g;
    ATOMIC_COUNT_INCREMENT
    factor.y -= rounding.y * textureOffset(edgeTex_linear, texcoord.zw, ivec2(-2, -1)).g;
    ATOMIC_COUNT_INCREMENT

    weights *= clamp(factor, 0.0, 1.0);
}

//-----------------------------------------------------------------------------
// Diagonal Search Functions

#if !defined(SMAA_DISABLE_DIAG_DETECTION)

/**
 * Allows to decode two binary values from a bilinear-filtered access.
 */
vec2 SMAADecodeDiagBilinearAccess(vec2 e) {
    // Bilinear access for fetching 'e' have a 0.25 offset, and we are
    // interested in the R and G edges:
    //
    // +---G---+-------+
    // |   x o R   x   |
    // +-------+-------+
    //
    // Then, if one of these edge is enabled:
    //   Red:   (0.75 * X + 0.25 * 1) => 0.25 or 1.0
    //   Green: (0.75 * 1 + 0.25 * X) => 0.75 or 1.0
    //
    // This function will unpack the values (mad + mul + round):
    // wolframalpha.com: round(x * abs(5 * x - 5 * 0.75)) plot 0 to 1
    e.r = e.r * abs(5.0 * e.r - 5.0 * 0.75);
    return round(e);
}

vec4 SMAADecodeDiagBilinearAccess(vec4 e) {
    e.rb = e.rb * abs(5.0 * e.rb - 5.0 * 0.75);
    return round(e);
}

/**
 * These functions allows to perform diagonal pattern searches.
 */
vec2 SMAASearchDiag1(vec2 texcoord, vec2 dir, out vec2 e) {
    vec4 coord = vec4(texcoord, -1.0, 1.0);
    vec3 t = vec3(pixel_size.xy, 1.0);
    while (coord.z < float(SMAA_MAX_SEARCH_STEPS_DIAG - 1) &&
           coord.w > 0.9) {
        coord.xyz = fma(t, vec3(dir, 1.0), coord.xyz);
        e = texture(edgeTex_linear, coord.xy).rg;
        ATOMIC_COUNT_INCREMENT
        coord.w = dot(e, vec2(0.5, 0.5));
    }
    return coord.zw;
}

vec2 SMAASearchDiag2(vec2 texcoord, vec2 dir, out vec2 e) {
    vec4 coord = vec4(texcoord, -1.0, 1.0);
    coord.x += 0.25 * pixel_size.x; // See @SearchDiag2Optimization
    vec3 t = vec3(pixel_size.xy, 1.0);
    while (coord.z < float(SMAA_MAX_SEARCH_STEPS_DIAG - 1) &&
           coord.w > 0.9) {
        coord.xyz = fma(t, vec3(dir, 1.0), coord.xyz);

        // @SearchDiag2Optimization
        // Fetch both edges at once using bilinear filtering:
        e = texture(edgeTex_linear, coord.xy).rg;
        ATOMIC_COUNT_INCREMENT
        e = SMAADecodeDiagBilinearAccess(e);

        coord.w = dot(e, vec2(0.5, 0.5));
    }
    return coord.zw;
}

/** 
 * Similar to SMAAArea, this calculates the area corresponding to a certain
 * diagonal distance and crossing edges 'e'.
 */
vec2 SMAAAreaDiag(vec2 dist, vec2 e, float offset) {
    vec2 texcoord = fma(vec2(SMAA_AREATEX_MAX_DISTANCE_DIAG, SMAA_AREATEX_MAX_DISTANCE_DIAG), e, dist);
    // We do a scale and bias for mapping to texel space:
    texcoord = fma(SMAA_AREATEX_PIXEL_SIZE, texcoord, 0.5 * SMAA_AREATEX_PIXEL_SIZE);
    // Diagonal areas are on the second half of the texture:
    texcoord.x += 0.5;
    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    ATOMIC_COUNT_INCREMENT
    return texture(areaTex, texcoord).rg;
}

/**
 * Conditional move:
 */
void SMAAMovc(bvec2 cond, inout vec2 variable, vec2 value) {
    if (cond.x) variable.x = value.x;
    if (cond.y) variable.y = value.y;
}

/**
 * This searches for diagonal patterns and returns the corresponding weights.
 */
vec2 SMAACalculateDiagWeights(vec2 texcoord, vec2 e) {
    vec2 weights = vec2(0.0, 0.0);

    // Search for the line ends:
    vec4 d;
    vec2 end;
    if (e.r > 0.0) {
        d.xz = SMAASearchDiag1(texcoord, vec2(-1.0,  -1.0), end);
        d.x += float(end.y > 0.9);
    } else
        d.xz = vec2(0.0, 0.0);
    d.yw = SMAASearchDiag1(texcoord, vec2(1.0, 1.0), end);

    if (d.x + d.y > 2.0) { // d.x + d.y + 1 > 3
        // Fetch the crossing edges:
        vec4 coords = fma(vec4(-d.x + 0.25, -d.x, d.y, d.y + 0.25), pixel_size.xyxy, texcoord.xyxy);
        vec4 c;
        c.xy = textureOffset(edgeTex_linear, coords.xy, ivec2(-1,  0)).rg;
        ATOMIC_COUNT_INCREMENT
        c.zw = textureOffset(edgeTex_linear, coords.zw, ivec2( 1,  0)).rg;
        ATOMIC_COUNT_INCREMENT
        c.yxwz = SMAADecodeDiagBilinearAccess(c.xyzw);

        // Merge crossing edges at each side into a single value:
        vec2 cc = fma(vec2(2.0, 2.0), c.xz, c.yw);

        // Remove the crossing edge if we didn't found the end of the line:
        SMAAMovc(bvec2(step(0.9, d.zw)), cc, vec2(0.0, 0.0));

        // Fetch the areas for this line:
        weights += SMAAAreaDiag(d.xy, cc, subsampleIndices.z);
    }

    // Search for the line ends:
    d.xz = SMAASearchDiag2(texcoord, vec2(-1.0, 1.0), end);
    ATOMIC_COUNT_INCREMENT
    if (textureOffset(edgeTex_linear, texcoord, ivec2(1, 0)).r > 0.0) {
        d.yw = SMAASearchDiag2(texcoord, vec2(1.0, -1.0), end);
        d.y += float(end.y > 0.9);
    } else
        d.yw = vec2(0.0, 0.0);

    if (d.x + d.y > 2.0) { // d.x + d.y + 1 > 3
        // Fetch the crossing edges:
        vec4 coords = fma(vec4(-d.x, d.x, d.y, -d.y), pixel_size.xyxy, texcoord.xyxy);
        vec4 c;
        c.x  = textureOffset(edgeTex_linear, coords.xy, ivec2(-1,  0)).g;
        ATOMIC_COUNT_INCREMENT
        c.y  = textureOffset(edgeTex_linear, coords.xy, ivec2( 0, 1)).r;
        ATOMIC_COUNT_INCREMENT
        c.zw = textureOffset(edgeTex_linear, coords.zw, ivec2( 1,  0)).gr;
        ATOMIC_COUNT_INCREMENT
        vec2 cc = fma(vec2(2.0, 2.0), c.xz, c.yw);

        // Remove the crossing edge if we didn't found the end of the line:
        SMAAMovc(bvec2(step(0.9, d.zw)), cc, vec2(0.0, 0.0));

        // Fetch the areas for this line:
        weights += SMAAAreaDiag(d.xy, cc, subsampleIndices.w).gr;
    }

    return weights;
}
#endif

//-----------------------------------------------------------------------------
// Blending Weight Calculation Pixel Shader (Second Pass)

vec4 blendingWeightCalculation(){
    vec4 weights = vec4(0.0, 0.0, 0.0, 0.0);
    vec2 e = texture(edgeTex_point, TexCoords).rg;
    ATOMIC_COUNT_INCREMENT

    if(e.g > 0.1){ // edge at top
        #if !defined(SMAA_DISABLE_DIAG_DETECTION)
        // Diagonals have both north and west edges, so searching for them in
        // one of the boundaries is enough.
        weights.rg = SMAACalculateDiagWeights(TexCoords, e);

        if (weights.r == -weights.g) { // weights.r + weights.g == 0.0
        #endif
        vec2 d;

        // distance to left
        vec3 coords;
        coords.x = SearchXLeft(offset[0].xy, offset[2].x);
        coords.y = offset[1].y; // offset[1].y = texcoord.y - 0.25 * pixel_size.y (@CROSSING_OFFSET)
        d.x = coords.x;

        float e1 = texture(edgeTex_linear, coords.xy).r;
        ATOMIC_COUNT_INCREMENT

        // distance to right
        coords.z = SearchXRight(offset[0].zw, offset[2].y);
        d.y = coords.z;

        // We want the distances to be in pixel units (doing this here allow to
        // better interleave arithmetic and memory accesses):
        d = abs(round(fma(pixel_size.zz, d, -PixCoords.xx)));
        vec2 sqrt_d = sqrt(d);

        // Fetch the right crossing edges:
        float e2 = textureOffset(edgeTex_linear, coords.zy, ivec2(1, 0)).r;
        ATOMIC_COUNT_INCREMENT

        // Ok, we know how this pattern looks like, now it is time for getting
        // the actual area:
        weights.rg = SMAAArea(sqrt_d, e1, e2, subsampleIndices.y);
        //weights.rg = SMAAArea(d, e1, e2, subsampleIndices.y);
        
        // Fix corners:
        coords.y = TexCoords.y;
        SMAADetectHorizontalCornerPattern(weights.rg, coords.xyzy, d);
        
        #if !defined(SMAA_DISABLE_DIAG_DETECTION)
        } else
            e.r = 0.0; // Skip vertical processing.
        #endif
    }

    if(e.r > 0.1){ // edge at left
        vec2 d;

        // distance to top
        vec3 coords;
        coords.y = SearchYUp(offset[1].xy, offset[2].z);
        coords.x = offset[0].x; // offset[1].x = texcoord.x - 0.25 * pixel_size.x;
        d.x = coords.y;

        float e1 = texture(edgeTex_linear, coords.xy).g;
        ATOMIC_COUNT_INCREMENT

        // distance to bottom
        coords.z = SearchYDown(offset[1].zw, offset[2].w);
        d.y = coords.z;

        // We want the distances to be in pixel units (doing this here allow to
        // better interleave arithmetic and memory accesses):
        d = abs(round(fma(pixel_size.ww, d, -PixCoords.yy)));
        vec2 sqrt_d = sqrt(d);

        float e2 = textureOffset(edgeTex_linear, coords.xz, ivec2(0, -1)).g;
        ATOMIC_COUNT_INCREMENT

        // Ok, we know how this pattern looks like, now it is time for getting
        // the actual area:
        weights.ba = SMAAArea(sqrt_d, e1, e2, subsampleIndices.x);
        //weights.ba = SMAAArea(d, e1, e2, subsampleIndices.x);
        
        // Fix corners:
        coords.x = TexCoords.x;
        SMAADetectVerticalCornerPattern(weights.ba, coords.xyxz, d);
    }
	return weights;
}

void main()
{
	FragColor = blendingWeightCalculation();
    //FragColor = texture(edgeTex_point, TexCoords);
    ATOMIC_COUNT_CALCULATE
}