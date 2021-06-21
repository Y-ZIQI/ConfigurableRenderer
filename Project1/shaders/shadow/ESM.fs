out float depth;

void main()
{
    depth = exp(80.0 * gl_FragCoord.z);
}