//++`shaders/shading/ShadingData.glsl`

#define SAMPLE_NUM 16
#define SAMPLE_BIAS 0.01

#define M_PI                3.14159265358979323846  // pi
#define M_2PI               6.28318530717958647693  // 2pi
#define M_1_PI              0.318309886183790671538 // 1/pi
#define M_1_2PI             0.15915494309   // 1/2pi

float Rand1(inout float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec2 Rand2(inout float p) {
    return vec2(Rand1(p), Rand1(p));
}

vec3 Rand3(inout float p) {
    return vec3(Rand1(p), Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
    vec2 uv = Rand2(s);
    float z = uv.x;
    float phi = uv.y * M_2PI;
    float sinTheta = sqrt(1.0 - z*z);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    pdf = M_1_2PI;
    return dir;
}

vec3 SampleHemisphere(inout float s) {
    vec3 uv = Rand3(s);
    float z = sqrt(1.0 - uv.x);
    float phi = uv.y * M_2PI;
    float sinTheta = sqrt(uv.x);
    vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    return dir * uv.z;
}

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
    float sign_ = sign(n.z);
    if (n.z == 0.0) {
        sign_ = 1.0;
    }
    float a = -1.0 / (sign_ + n.z);
    float b = n.x * n.y * a;
    b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
    b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

float LinearizeDepth(float depth, vec2 inv_nf)
{
    float z = depth * 2.0 - 1.0; // 回到NDC
    return 2.0 / (inv_nf.x - z * inv_nf.y);    
}

float ambientOcclusion(ShadingData sd, float range, float threshold, mat4 transform, vec2 inv_nf, sampler2D positionTex){
    //float seed = InitRand(vec2(1.0));
    float seed = InitRand(1000.0 * sd.posW.xy);
    float sum = 0.0, mindep, dist;
    vec3 randPos, samplePos, objPos, b1, b2, ndc;
    vec4 ssPos;

    LocalBasis(sd.N, b1, b2);
    mat3 TBN = mat3(b1, b2, sd.N);
    for(int i = 0;i < SAMPLE_NUM;i++){
        randPos = SampleHemisphere(seed) * range;
        samplePos = TBN * randPos + sd.posW;
        ssPos = transform * vec4(samplePos, 1.0);
        ndc = clamp((ssPos.xyz / ssPos.w + 1.0) / 2.0, 0.0, 1.0);
        mindep = texture(positionTex, ndc.xy).w;
        ndc.z = LinearizeDepth(ndc.z, inv_nf);
        sum += step(ndc.z, mindep + SAMPLE_BIAS) + step(mindep + threshold, ndc.z);
        //objPos = texture(positionTex, ndc.xy).xyz;
        //dist = dot(samplePos - objPos, sd.V);
        //sum += step(-SAMPLE_BIAS, dist) + step(threshold, -dist);
        //mindep = texture(depthTex, ndc.xy).r;
        //sum += step(ndc.z, mindep) + step(mindep + threshold, ndc.z);
    }
    return sum / float(SAMPLE_NUM);
}