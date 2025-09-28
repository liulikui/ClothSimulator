#ifndef XPBD_SOLVER_H
#define XPBD_SOLVER_H

#include <vector>
#include <DirectXMath.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include "Particle.h"

class Cloth;
class Constraint;

// 为了方便使用，创建一个命名空间别名
namespace dx = DirectX;

// XPBD (Extended Position Based Dynamics) 求解器
// 一种基于位置的物理模拟系统，特别适合处理约束
class XPBDSolver
{
public:
    // 构造函数
    // 参数
    //   cloth - 布料
    XPBDSolver(Cloth* cloth)
        : m_cloth(cloth)
    {
    }
    
    // 析构函数
    ~XPBDSolver()
    {
    }

    // 模拟一步
    // 执行一次完整的XPBD模拟步骤，包括预测、约束求解和位置校正
    void Step(float deltaTime);
    
private:
    // 保存单帧初始位置（用于计算帧末总速度）
    void BeginStep();

    // 预测粒子的位置（考虑外力）
    void PredictPositions(float deltaTime);
    
    // 求解所有约束
    void SolveConstraints(float deltaTime);

    // 求解单个约束
    void SolveConstraint(Constraint* constraint, float deltaTime);
    
    // 更新粒子的速度
    void UpdateVelocities(float deltaTime);

    // 将当前帧最终速度作为下一帧初始速度
    void EndStep(float deltaTime);

protected:
    Cloth* m_cloth;
};

#endif // XPBD_SOLVER_H
 
