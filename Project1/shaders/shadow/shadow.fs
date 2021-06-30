//#define SHADOW_SOFT_ESM

#ifdef SHADOW_SOFT_ESM
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