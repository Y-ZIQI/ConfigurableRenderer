# ConfigurableRenderer

OpenGL实现的一个渲染管线，可以自由调节场景里的模型材质，光照属性，摄像机属性等。并添加了一些渲染相关的可调节的参数，包括：

| 名称       | 参数           | 取值                   |
| ---------- | -------------- | ---------------------- |
| 分辨率     | 占整个屏幕比例 | 60%, 80%, 100%         |
| Shadow Map | 阴影贴图分辨率 | 25%, 50%, 100%         |
| SSAO       | 采样数         | 0, 4, 16               |
| Shading    | 着色模型       | GGX-based PBR, PBR+IBL |
| 反射       | SSR            | Off, Low, High         |
| SMAA抗锯齿 | 查询步数       | 0, 4, 32               |
| 特效       | 泛光           | Off, Low, High         |

## Demo

![](src/demo1.png)

![](src/demo2.png)

![](src/demo3.png)

![](src/demo4.png)

![](src/demo5.png)

![](src/demo6.png)

![](src/demo7.png)

![](src/demo8.png)

## Dependency

- glfw3
- assimp
- stb_image
- imgui
- jsonxx

## Test Scenes

- SunTemple: 
  - 从nvidia官网上下载SunTemple模型数据包[下载地址](https://developer.nvidia.com/sun-temple)
  - 将resources下的SunTemple文件夹与模型文件夹合并
  - 加载resources/SunTemple/SunTemple.json
- Bistro:
  - 从nvidia官网上下载Bistro模型数据包[下载地址](https://developer.nvidia.com/bistro)
  - 将resources下的Bistro文件夹与模型文件夹合并
  - 加载resources/Bistro/BistroExterior.json或resources/Bistro/BistroInterior_Wine.json
- Arcade:
  - 直接加载resources/Arcade/Arcade.json