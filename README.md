# XPBD 布料模拟器

这是一个基于XPBD (Extended Position-Based Dynamics) 算法的布料模拟器，使用DirectX 12作为渲染API。模拟器实现了经典的布料掉落到球体上的物理效果，包含完整的物理模拟和3D渲染系统。

## 项目结构

```
ClothSimulator/
├── .gitignore           # Git忽略文件配置
├── CMakeLists.txt       # CMake构建脚本
├── README.md            # 项目说明文档
├── src/                 # 源代码目录
│   ├── Main.cpp         # 主程序文件
│   ├── Main.h           # 主程序头文件
│   ├── Particle.h       # 粒子类定义
│   ├── Constraint.h     # 约束基类定义
│   ├── DistanceConstraint.h # 距离约束实现
│   ├── SphereCollisionConstraint.h # 球体碰撞约束实现
│   ├── XPBDSolver.h     # XPBD求解器实现
│   ├── Cloth.h          # 布料类定义
│   ├── Cloth.cpp        # 布料类实现
│   ├── Camera.h         # 相机类头文件
│   ├── Camera.cpp       # 相机类实现
│   ├── Mesh.h           # 网格类定义
│   ├── Mesh.cpp         # 网格类实现
│   ├── Primitive.h      # 图元基类定义
│   ├── Primitive.cpp    # 图元基类实现
│   ├── Sphere.h         # 球体类定义
│   ├── Sphere.cpp       # 球体类实现
│   ├── Scene.h          # 场景类定义
│   ├── Scene.cpp        # 场景类实现
│   ├── IRALDevice.h     # 渲染抽象层设备接口
│   ├── DX12RALDevice.h  # DirectX 12设备实现头文件
│   ├── DX12RALDevice.cpp # DirectX 12设备实现
│   ├── RALCommandList.h # 渲染命令列表接口
│   ├── DX12RALCommandList.h # DirectX 12命令列表实现头文件
│   ├── DX12RALCommandList.cpp # DirectX 12命令列表实现
│   ├── RALResource.h    # 渲染资源接口
│   ├── DX12RALResource.h # DirectX 12资源实现头文件
│   ├── DX12RALResource.cpp # DirectX 12资源实现
│   ├── DataFormat.h     # 数据格式定义
│   └── TSharePtr.h      # 智能指针实现
└── xpbd_solver.pseudo   # XPBD求解器伪代码参考
```

## 依赖项

项目使用以下库和工具：
- DirectX 12 SDK
- DirectXMath (数学库)
- Windows SDK
- CMake (3.10或更高版本，构建系统)
- Visual Studio 2019或更高版本

## 构建和运行

### Windows (使用Visual Studio)

1. 确保您已安装CMake (3.10或更高版本) 和Visual Studio (推荐2019或更高版本)。
2. 打开命令提示符或PowerShell，导航到项目目录。
3. 创建并进入构建目录：
   ```
   mkdir build
   cd build
   ```
4. 运行CMake配置：
   ```
   cmake ..
   ```
5. 打开生成的解决方案文件，在Visual Studio中选择"Release"配置，并构建解决方案。
6. 运行生成的可执行文件。

## 使用说明

- **相机控制**：
  - W/S/A/D键：移动摄像机位置
  - 鼠标右键 + 移动：旋转相机视角
  - 鼠标滚轮：缩放相机距离
- **程序控制**：
  - F9键：切换调试输出开关
  - ESC键：退出程序
- **布料模拟**：
  - 程序启动后，布料会在重力作用下掉落到球体上
  - 默认情况下，布料的左上角和右上角粒子是固定的
- **窗口信息**：
    - 窗口尺寸：默认800×600像素，可通过命令行参数自定义或切换至全屏模式
    - 窗口标题会显示当前帧率、迭代次数、布料分辨率、LRA约束状态、LRAMaxStretch值和粒子质量（格式："XPBD Cloth Simulator (DirectX 12) [X FPS, Y Iter, WxH Res, LRA:ON/OFF, MaxStretch:Z, Mass:M]"），其中W和H分别表示布料的宽度和高度分辨率，LRA:ON表示启用LRA约束，LRA:OFF表示禁用LRA约束，Z表示LRA约束的最大拉伸量，M表示每个粒子的质量

## 命令行参数

程序支持以下命令行参数：

| 参数 | 描述 | 默认值 |
|------|------|--------|
| `-help` | 显示帮助信息 | 无 |
| `-debug` | 启用调试模式，输出详细日志信息 | 禁用 |
| `-maxFrames:X` | 设置最大帧数限制，达到后程序自动退出 | 无限制 |
| `-iteratorCount:X` | 设置XPBD求解器的迭代次数，影响物理模拟精度和性能 | 50 |
| `-widthResolution:X` | 设置布料宽度方向的粒子数量（分辨率），影响布料细节和性能 | 40 |
| `-heightResolution:X` | 设置布料高度方向的粒子数量（分辨率），影响布料细节和性能 | 40 |
| `-addLRAConstraint:X` | 设置是否添加LRA约束，X可以是true/false/1/0/yes/no | true |
| `-LRAMaxStretch:X` | 设置LRA约束最大拉伸量，X为数值 | 0.01 |
| `-mass:X` | 设置每个粒子的质量，X为数值 | 1.0 |
| `-fullscreen` | 以全屏模式启动程序 | 禁用 |
| `-winWidth:X` | 设置窗口宽度，X为数字，不能超过系统分辨率 | 800 |
| `-winHeight:X` | 设置窗口高度，X为数字，不能超过系统分辨率 | 600 |

示例用法：
```
XPBDClothSimulator.exe -debug -iteratorCount:100
XPBDClothSimulator.exe -maxFrames:5000 -iteratorCount:30
XPBDClothSimulator.exe -widthResolution:60 -heightResolution:60 -debug
XPBDClothSimulator.exe -widthResolution:20 -heightResolution:20 -iteratorCount:10
XPBDClothSimulator.exe -LRAMaxStretch:0.05
XPBDClothSimulator.exe -addLRAConstraint:true -LRAMaxStretch:0.05
XPBDClothSimulator.exe -mass:2.5
XPBDClothSimulator.exe -addLRAConstraint:true -mass:0.5 -LRAMaxStretch:0.02
XPBDClothSimulator.exe -fullscreen
XPBDClothSimulator.exe -winWidth:1280 -winHeight:720
XPBDClothSimulator.exe -fullscreen -debug
```

## 许可证

[MIT License](LICENSE)