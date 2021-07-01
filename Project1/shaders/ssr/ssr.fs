//++`shaders/shading/defines.glsl`

out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 camera_pos;
uniform vec4 camera_params;
uniform mat4 camera_vp;

uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D specularTex;
uniform sampler2D colorTex;

#define INIT_STEP 1.0
#define MAX_STEPS 32
#define THRES 0.01

float LinearizeDepth(float depth)
{
    //float z = depth * 2.0 - 1.0;
    float z = depth;
    return 2.0 / (camera_params.z - z * camera_params.w);    
}

// From world pos to [screen.xy, linear_depth]
vec3 getCoord(vec3 posw){
    vec4 buf = camera_vp * vec4(posw, 1.0);
    buf.xyz /= buf.w;
    vec3 pos = vec3(buf.xy * 0.5 + 0.5, LinearizeDepth(buf.z));
    return pos;
}

bool rayTrace(vec3 pos, vec3 dir, out vec2 hitPos, out int stps){
    bool intersect = false;
    vec3 current = pos;
    vec3 curruv = getCoord(pos);
    vec3 dir1 = dir * INIT_STEP;
    float dep, d1 = 0.0, d2, maxdep = length(dir1);
    stps = MAX_STEPS;
    for(int i = 0;i < MAX_STEPS;i++){
        ATOMIC_COUNT_INCREMENT
        dep = texture(positionTex, curruv.xy).w;
        if(curruv.z > dep + THRES){
            if(curruv.z < dep + clamp(maxdep, 0.5, 1.0)){
                if(maxdep < THRES){
                    intersect = true;
                    d2 = curruv.z - dep;
                    current = current - dir1 * d2 / (d1 + d2);
                    hitPos = getCoord(current).xy;
                    stps = i + 1;
                    break;
                }
                dir1 *= 0.5;
                maxdep *= 0.5;
                current -= dir1;
                curruv = getCoord(current);
                continue;
            }else{
                dir1 = dir * INIT_STEP;
                maxdep = length(dir1);
            }
        }else{
            d1 = dep - curruv.z;
        }
        current += dir1;
        curruv = getCoord(current);
        if(max(curruv.x, curruv.y) >= 1.0 || min(curruv.x, curruv.y) <= 0.0){
            stps = i + 1;
            break;
        }
    }
    return intersect;
}

void main(){
    ATOMIC_COUNT_INCREMENT
    vec3 normal = texture(normalTex, TexCoords).rgb;
    ATOMIC_COUNT_INCREMENT
    vec3 color = texture(colorTex, TexCoords).rgb;
    if(dot(normal, normal) == 0.0){
        FragColor = vec4(color, 1.0);
    }else{
        ATOMIC_COUNT_INCREMENT
        vec4 position = texture(positionTex, TexCoords);
        ATOMIC_COUNT_INCREMENT
        vec3 specular = texture(specularTex, TexCoords).rgb;
        vec3 view = normalize(position.xyz - camera_pos);
        vec3 refv = reflect(view, normal);

        float reflection = 1.0 - specular.g;
        float threshold = 0.5, max_ssr_ratio = 0.5;
        if(reflection > threshold){
            vec2 hitpos;
            int stps;
            bool inter = rayTrace(position.xyz, refv, hitpos, stps);
            if(inter){
                ATOMIC_COUNT_INCREMENT
                FragColor = vec4(texture(colorTex, hitpos).rgb * max_ssr_ratio * reflection + color * (1.0 - max_ssr_ratio * reflection), 1.0);
            }else{
                FragColor = vec4(color, 1.0);
            }
        }else{
            FragColor = vec4(color, 1.0);
        }
    }
}
/*intersect = true;
d2 = curruv.z - dep;
current = current - dir1 * d2 / (d1 + d2);
hitPos = getCoord(current).xy;
//hitPos = curruv.xy;
stps = i + 1;
break;*/
/*vec3 spec = texture(specularTex, TexCoords).rgb;
if(spec.g < 0.2)
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);*/
//FragColor = vec4(color, 1.0);
/*if(gl_FragCoord.y < 500){
    FragColor = vec4(vec3(getCoord(position.xyz).z)*0.1, 1.0);
}else if(gl_FragCoord.y < 1000){
    FragColor = vec4(vec3(position.w)*0.1, 1.0);
}*/
//if(abs(getCoord(position.xyz).z - position.w) > 0.001)
//    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
//FragColor = vec4(getCoord(position.xyz), 1.0);