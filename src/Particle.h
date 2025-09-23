#ifndef PARTICLE_H
#define PARTICLE_H

#include <DirectXMath.h>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

class Particle
{
public:
    // 构造函数
    // 参数：
    //   pos - 粒子的初始位置
    //   m - 粒子的质量
    //   isStatic - 粒子是否为固定（不可移动）
    Particle(const dx::XMFLOAT3& pos = dx::XMFLOAT3(0.0f, 0.0f, 0.0f), float m = 1.0f, bool isStatic = false)
        : position(pos), oldPosition(pos), velocity(dx::XMFLOAT3(0.0f, 0.0f, 0.0f)), force(dx::XMFLOAT3(0.0f, 0.0f, 0.0f)),
          mass(m), inverseMass(1.0f / m), isStatic(isStatic)
    {
        if (isStatic) {
            inverseMass = 0.0f; // 固定粒子的质量倒数为0，表示不受力
        }
    }

    // 应用力
    // 参数：
    //   f - 要应用的力向量
    void ApplyForce(const dx::XMFLOAT3& f)
    {
        if (!isStatic) 
        {
            // 将力转换为XMVECTOR进行计算
            dx::XMVECTOR forceVector = dx::XMLoadFloat3(&force);
            dx::XMVECTOR appliedForce = dx::XMLoadFloat3(&f);
            
            // 叠加力
            forceVector = dx::XMVectorAdd(forceVector, appliedForce);
            
            // 将结果转换回XMFLOAT3
            dx::XMStoreFloat3(&force, forceVector);
        }
    }

    // 重置力
    void ResetForce()
    {
        force = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    // 公共成员变量
    dx::XMFLOAT3 position;     // 当前位置
    dx::XMFLOAT3 oldPosition;  // 上一帧的位置
    dx::XMFLOAT3 velocity;     // 速度
    dx::XMFLOAT3 force;        // 作用在粒子上的力
    float mass;                // 质量
    float inverseMass;         // 质量的倒数（用于加速度计算）
    bool isStatic;             // 是否为固定粒子
};

#endif // PARTICLE_H
