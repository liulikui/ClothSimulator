#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <DirectXMath.h>
#include "Particle.h"

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 约束基类，所有类型的约束都应继承自这个类
class Constraint
{
public:
    // 构造函数
    // 参数：
    //   compliance - 柔度（与刚度成反比，值越小刚度越大）
    //   damping - 阻尼系数
    Constraint(float compliance, float damping)
        : m_lambda(0.0f)
        , m_compliance(compliance)
        , m_damping(damping)
    {}

    // 虚析构函数
    virtual ~Constraint() = default;

    // 获取约束类型
    virtual const char* GetConstraintType() const = 0;

    // 计算约束偏差和约束梯度
    // 参数：
    //   gradients - 存储每个受约束粒子的梯度向量的向量
    // 返回：约束偏差值C(x)
    virtual float ComputeConstraintAndGradient(dx::XMFLOAT3* gradients) const = 0;
    
    // 获取受此约束影响的所有粒子的数量
    // 返回：受约束影响的粒子数量
    virtual uint32_t GetParticlesCount() const = 0;

    // 获取受此约束影响的所有粒子
    // 返回：受约束影响的粒子的数组
    virtual Particle** GetParticles() = 0;

    // 获取受此约束影响的所有粒子
    // 返回：受约束影响的粒子的数组
    virtual const Particle** GetParticles() const = 0;

    // 设置约束的柔度
    // 参数：
    //   c - 新的柔度值
    inline void SetCompliance(float c)
    {
        m_compliance = c;
    }
    
    // 获取约束的柔度
    // 返回：柔度值
    inline float GetCompliance() const
    {
        return m_compliance;
    }
    
    // 设置约束的阻尼系数
    // 参数：
    //   d - 新的阻尼系数
    inline void SetDamping(float d)
    {
        m_damping = d;
    }

    // 获取阻尼系数
    // 返回：阻尼系数
    inline float GetDamping() const
    {
        return m_compliance;
    }

    // 设置拉格朗日乘子
    // 参数：
    //   l - 新的拉格朗日乘子
    inline void SetLambda(float l)
    {
        m_lambda = l;
    }

    // 获取拉格朗日乘子
    // 返回：拉格朗日乘子
    inline float GetLambda() const
    {
        return m_lambda;
    }

protected:
    float m_lambda;         // 拉格朗日乘子
    float m_compliance;     // 柔度（与刚度成反比）
    float m_damping;        // 阻尼系数，控制约束方向的阻尼强度，0为无阻尼
};

#endif // CONSTRAINT_H

