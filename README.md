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
  - 鼠标右键 + 移动：旋转相机视角
  - 鼠标滚轮：缩放相机距离
- **程序控制**：
  - F9键：切换调试输出开关
  - ESC键：退出程序
- **布料模拟**：
  - 程序启动后，布料会在重力作用下掉落到球体上
  - 默认情况下，布料的左上角和右上角粒子是固定的
- **窗口信息**：
  - 窗口尺寸：800×600像素
  - 窗口标题会显示当前帧率（格式："XPBD Cloth Simulator (DirectX 12) [X FPS]"）

## 技术细节

- **XPBD算法**：实现了基于位置的动态模拟，使用拉格朗日乘子来满足约束条件
- **布料结构**：使用粒子和距离约束来模拟布料，包含结构约束、剪切约束和弯曲约束
- **碰撞检测**：实现了布料与球体之间的碰撞检测和响应
- **渲染系统**：使用DirectX 12进行3D渲染，包含完整的渲染抽象层(RAL)
- **光照系统**：采用平行光源，位置为(-10.0f, 30.0f, -10.0f)，白色光源(1.0f, 1.0f, 1.0f, 1.0f)
- **窗口系统**：使用Windows API创建800×600像素的窗口，支持实时帧率显示
- **命令行参数**：支持调试模式、最大帧数限制等选项 (使用 -help 查看完整参数列表)

## 自定义修改

您可以通过修改以下参数来自定义模拟效果：

1. 在 `Main.cpp` 中：
   - 修改布料的初始位置、尺寸和粒子间距
   - 调整球体的位置和半径
   - 更改相机的初始位置和窗口设置
   - 调整光源位置和颜色

2. 在 `Cloth.h` 和 `Cloth.cpp` 中：
   - 调整约束的柔度参数
   - 修改布料固定点的位置
   - 调整布料粒子质量和物理属性

3. 在 `XPBDSolver.h` 中：
   - 更改时间步长
   - 调整约束迭代次数
   - 修改重力加速度

## 许可证

[MIT License](LICENSE)