//++`shaders/shading/defines.glsl`

out vec4 outColor;

in vec2 TexCoords;

uniform bool horizontal;
uniform sampler2D colorTex;
uniform sampler2D horizontalTex;

const float weights[] = {
    1.0, 
    0.39099131515943186, 0.304504342420284, 
    0.2688318247165084, 0.22756122669152126, 0.1380228609502245, 
    0.21610594100794114, 0.19071282356963737, 0.13107487896736597, 0.07015932695902607, 
    0.18631524637462044, 0.16858500647034522, 0.12489084452697706, 0.07575012632301532, 0.03761639949235221, 
    0.16683050047405476, 0.15349147015096878, 0.11953927714835563, 0.07880514840098747, 0.04397604247537524, 0.020772811587285438, 
    0.15284265059690583, 0.14230610312208586, 0.11485778133701453, 0.08036283530534685, 0.048742523514129735, 0.025628235144021384, 0.011681196278948813, 
    0.14214005255089007, 0.13352822211530005, 0.11069878423244384, 0.08098896064984798, 0.05229040310050089, 0.029794173584922007, 0.01498145131804646, 0.0066479787234937655, 
    0.13357122034787947, 0.12635296066150858, 0.1069554720857897, 0.08101504039622084, 0.054912770343056964, 0.03330627882282267, 0.01807689893804027, 0.008779439778980491, 0.0038155287996407016, 
    0.12647984122429223, 0.12031134657872519, 0.10355293565474837, 0.08064710737726083, 0.05683105603902189, 0.036237081216874946, 0.020906977136818293, 0.01091439911911511, 0.0051555970871359725, 0.0022035791781532758
};
const int part[] = {
    0, 1, 3, 6, 10, 15, 21, 28, 36, 45
};

const int ksize = 5;

void main()
{
    vec4 result;
    int start = part[ksize - 1];
    if(horizontal){
        vec4 color = texture(colorTex, TexCoords);
        result = color * weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(colorTex, TexCoords, ivec2(i, 0)) * weights[start + i];
            result += textureOffset(colorTex, TexCoords, ivec2(-i, 0)) * weights[start + i];
        }
    }else{
        result = texture(horizontalTex, TexCoords) * weights[start];
        for(int i = 1;i < ksize;i++){
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, i)) * weights[start + i];
            result += textureOffset(horizontalTex, TexCoords, ivec2(0, -i)) * weights[start + i];
        }
    }
    outColor = result;
}