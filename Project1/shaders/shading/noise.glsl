#ifndef __NOISE
#define __NOISE

/* [0, 1] */
float random(float x)
{
    float y = fract(sin(x)*43758.5453123);
    return y;
}

/* [0, 1] */
float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

/* [0, 1] */
vec2 random2(vec2 st){
    st = vec2( dot(st,vec2(127.1,311.7)),
              dot(st,vec2(269.5,183.3)) );
    return fract(sin(st)*43758.5453123);
}

/* [0, 1] */
vec3 random3(vec3 c) {
    float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
    vec3 r;
    r.z = fract(512.0*j);
    j *= .125;
    r.x = fract(512.0*j);
    j *= .125;
    r.y = fract(512.0*j);
    return r;
}

#endif