# ConfigurableRenderer

OpenGL实现的一个渲染管线，仿照Nvidia的Falcor。并添加了一些可调节的参数，包括：

| 名称       | 参数           | 取值                                |
| ---------- | -------------- | ----------------------------------- |
| 分辨率     | 占整个屏幕比例 | 60%，80%，100%                      |
| Shadow Map | 阴影贴图分辨率 | 512，1024，2048                     |
| SSAO       | 采样数         | 0，4，16                            |
| Shading    | 着色模型       | Blinn-Phong，GGX-based PBR，PBR+IBL |
| 反射       | 未实现         |                                     |
| SMAA抗锯齿 | 查询步数       | 4，16，64                           |

## Demo

![](D:\project\OpenGL\pipeline\Project1\src\1.png)

![](D:\project\OpenGL\pipeline\Project1\src\2.png)

![](D:\project\OpenGL\pipeline\Project1\src\3.png)

![](D:\project\OpenGL\pipeline\Project1\src\4.png)

## Dependency

glfw3

assimp

nanogui