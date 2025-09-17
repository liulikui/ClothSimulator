# XPBD 布料模拟器

这是一个基于XPBD (Extended Position-Based Dynamics) 算法的布料模拟器，使用DirectX 12作为渲染API。模拟器实现了经典的布料掉落到球体上的效果。

## 项目结构

```
ClothSimulator/
├── CMakeLists.txt       # CMake构建脚本
├── README.md            # 项目说明文档
├── build/               # 构建目录
└── src/                 # 源代码目录
    ├── Main.cpp         # 主程序文件
    ├── Particle.h       # 粒子类定义
    ├── Constraint.h     # 约束基类定义
    ├── DistanceConstraint.h # 距离约束实现
    ├── SphereCollisionConstraint.h # 球体碰撞约束实现
    ├── XPBDSolver.h     # XPBD求解器实现
    ├── Cloth.h          # 布料类实现
    ├── Cloth.cpp        # 布料类实现
    ├── DX12Renderer.h   # DirectX 12渲染器头文件
    ├── DX12Renderer.cpp # DirectX 12渲染器实现
    ├── Camera.h         # 相机类头文件
    └── Camera.cpp       # 相机类实现
```

## 依赖项

项目使用以下库：
- DirectX 12 SDK
- DirectXMath (数学库)
- Windows SDK
- CMake (构建系统)

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
5. 打开生成的解决方案文件 `XPBDClothSimulator.sln`。
6. 在Visual Studio中选择"Release"配置，并构建解决方案。
7. 运行生成的可执行文件 `XPBDClothSimulator.exe` (位于 `bin/Release/` 目录下)。

## 使用说明

- **相机控制**：
  - 使用WASD键移动相机
  - 使用鼠标移动来旋转视角
  - 使用空格键和左Shift键上下移动相机
- **布料模拟**：
  - 程序启动后，布料会在重力作用下掉落到球体上
  - 默认情况下，布料的左上角和右上角粒子是固定的

## 技术细节

- **XPBD算法**：实现了基于位置的动态模拟，使用拉格朗日乘子来满足约束条件
- **布料结构**：使用粒子和距离约束来模拟布料
- **碰撞检测**：实现了布料与球体之间的碰撞检测和响应
- **渲染**：使用DirectX 12进行3D渲染，包含光照效果

## 自定义修改

您可以通过修改以下参数来自定义模拟效果：

1. 在 `Main.cpp` 中：
   - 修改布料的初始位置、尺寸和粒子间距
   - 调整球体的位置和半径
   - 更改相机的初始位置

2. 在 `Cloth.h` 和 `Cloth.cpp` 中：
   - 调整约束的柔度参数
   - 修改布料固定点的位置

3. 在 `XPBDSolver.h` 中：
   - 更改时间步长
   - 调整约束迭代次数
   - 修改重力加速度

## 许可证

[MIT License](LICENSE)