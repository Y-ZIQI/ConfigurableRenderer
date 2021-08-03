//#define SHADOW_SOFT_EVSM

#ifdef SHADOW_SOFT_EVSM
out vec4 depth;

in vec4 FragPos;

const float posi_c = 40.0;
const float nega_c = 40.0;

void main(){
    float z = 0.5 * FragPos.z / FragPos.w + 0.5;
    depth.r = exp(z * posi_c);
    depth.g = exp(z * posi_c * 2.0);
    depth.b = -exp(-z * nega_c);
    depth.a = exp(-z * nega_c * 2.0);
}
#elif defined(SHADOW_SOFT_ESM)
out float depth;

in vec4 FragPos;

void main()
{
    float z = 0.5 * FragPos.z / FragPos.w + 0.5;
    depth = exp(80.0 * z);
}
#else
out float depth;

in vec4 FragPos;

void main()
{
    depth = 0.5 * FragPos.z / FragPos.w + 0.5;
}
#endif