//++`shaders/shading/defines.glsl`

out vec2 FragColor;

in vec2 TexCoords;

uniform sampler2D Texture;

const vec2 ones = vec2(1.0, 1.0);
const vec2 threshold = 0.1*ones;
const vec3 weights = vec3(0.2126, 0.7152, 0.0722);

float lumia(vec4 a){
	return dot(weights, a.rgb);
}

vec2 edgeDetection(){
	vec4 color = texture(Texture, TexCoords);
    ATOMIC_COUNT_INCREMENT
	vec4 topcolor = textureOffset(Texture, TexCoords, ivec2(0, 1));
    ATOMIC_COUNT_INCREMENT
	vec4 top2color = textureOffset(Texture, TexCoords, ivec2(0, 2));
    ATOMIC_COUNT_INCREMENT
	vec4 leftcolor = textureOffset(Texture, TexCoords, ivec2(-1, 0));
    ATOMIC_COUNT_INCREMENT
	vec4 left2color = textureOffset(Texture, TexCoords, ivec2(-2, 0));
    ATOMIC_COUNT_INCREMENT
	vec4 bottomcolor = textureOffset(Texture, TexCoords, ivec2(0, -1));
    ATOMIC_COUNT_INCREMENT
	vec4 rightcolor = textureOffset(Texture, TexCoords, ivec2(1, 0));
    ATOMIC_COUNT_INCREMENT

	float L = lumia(color);
	float Lleft = lumia(leftcolor);
	float Lleft2 = lumia(left2color);
	float Ltop = lumia(topcolor);
	float Ltop2 = lumia(top2color);
	float Lright = lumia(rightcolor);
	float Lbottom = lumia(bottomcolor);

    vec2 delta1 = abs(vec2(L, L) - vec2(Lleft, Ltop));
	vec2 edges = step(threshold, delta1);

    if(dot(edges, ones) == 0.0){
       discard;
    }

    vec4 delta2 = abs(vec4(L, L, Lleft2, Ltop2) - vec4(Lright, Lbottom, Lleft, Ltop));
    float right_bottom_max = max(delta2.x, delta2.y);
    vec2 localmax = vec2(max(right_bottom_max, max(delta1.y, delta2.z)), max(right_bottom_max, max(delta1.x, delta2.z)));

    edges *= step(localmax * 0.5, delta1);
	return edges;
}

void main()
{
	FragColor = edgeDetection();
}