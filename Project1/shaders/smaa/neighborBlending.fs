//++`shaders/shading/defines.glsl`

out vec4 FragColor;

in vec2 TexCoords;
in vec4 offset;

uniform sampler2D blendTex;
uniform sampler2D screenTex;
uniform float width;
uniform float height;

const vec4 pixel_size = vec4(1.0/width, 1.0/height, width, height);

const vec4 ones = vec4(1.0, 1.0, 1.0, 1.0);

/**
 * Conditional move:
 */
void SMAAMovc(bvec2 cond, inout vec2 variable, vec2 value) {
    if (cond.x) variable.x = value.x;
    if (cond.y) variable.y = value.y;
}

void SMAAMovc(bvec4 cond, inout vec4 variable, vec4 value) {
    SMAAMovc(cond.xy, variable.xy, value.xy);
    SMAAMovc(cond.zw, variable.zw, value.zw);
}

vec4 neighborhoodBlending(){
    vec4 a;
    a.x = texture(blendTex, offset.xy).a; // blend with Right
    a.y = texture(blendTex, offset.zw).g; // blend with Bottom
    a.wz = texture(blendTex, TexCoords).xz; // blend with Top / Left
    ATOMIC_COUNTER_I_INCREMENT(2)

    if(dot(a, ones) < 1e-5){
        vec4 color = texture(screenTex, TexCoords);
        return color;
    }else{
        bool h = max(a.x, a.z) > max(a.y, a.w); // max(horizontal) > max(vertical)
        
        // Calculate the blending offsets:
        vec4 blendingOffset = vec4(0.0, -a.y, 0.0, -a.w);
        vec2 blendingWeight = a.yw;
        SMAAMovc(bvec4(h, h, h, h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
        SMAAMovc(bvec2(h, h), blendingWeight, a.xz);
        blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0));

        // Calculate the texture coordinates:
        vec4 blendingCoord = fma(blendingOffset, vec4(pixel_size.xy, -pixel_size.xy), TexCoords.xyxy);

        // We exploit bilinear filtering to mix current pixel with the chosen
        // neighbor:
        vec4 color = blendingWeight.x * texture(screenTex, blendingCoord.xy);
        color += blendingWeight.y * texture(screenTex, blendingCoord.zw);
        ATOMIC_COUNTER_I_INCREMENT(0)
        
        return color;
    }
}

void main()
{
	FragColor = neighborhoodBlending();
	//FragColor = texture(blendTex, TexCoords);
}