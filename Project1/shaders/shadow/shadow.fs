//#define SHADOW_SOFT_ESM
//#define SHADOW_SOFT_EVSM

#ifdef SHADOW_SOFT_EVSM
out vec4 depth;

const float posi_c = 40.0;
const float nega_c = 40.0;

void main(){
    /*depth.r = gl_FragCoord.z; // VSM
    depth.g = gl_FragCoord.z * gl_FragCoord.z;*/
    depth.r = exp(gl_FragCoord.z * posi_c);
    depth.g = exp(gl_FragCoord.z * posi_c * 2.0);
    depth.b = -exp(-gl_FragCoord.z * nega_c);
    depth.a = exp(-gl_FragCoord.z * nega_c * 2.0);
}
#elif defined(SHADOW_SOFT_ESM)
out float depth;

void main()
{
    depth = exp(80.0 * gl_FragCoord.z);
}
#else
out float depth;

void main()
{
    depth = gl_FragCoord.z;
}
#endif