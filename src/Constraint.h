#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <vector>
#include <DirectXMath.h>
#include "Particle.h"

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 约束基类，所有类型的约束都应继承自这个类
class Constraint {
public:
    // 虚析构函数
    virtual ~Constraint() = default;
    
    // 构造函数
    Constraint() : lambda(0.0f), compliance(1e-6f)
    {}
    
    // 拉格朗日乘子
    float lambda;
    
    // 计算约束偏差（约束函数的值）
    // 返回：约束偏差值C(x)
    virtual float ComputeConstraintValue() const = 0;
    
    // 计算约束梯度
    // 参数：
    //   gradients - 存储每个受约束粒子的梯度向量的向量
    virtual void ComputeGradient(std::vector<dx::XMFLOAT3>& gradients) const = 0;
    
    // 获取受此约束影响的所有粒子
    // 返回：粒子指针的向量
    virtual std::vector<Particle*> GetParticles() = 0;
    
    // 获取受此约束影响的所有粒子（const版本）
    // 返回：const粒子指针的向量
    virtual std::vector<const Particle*> GetParticles() const = 0;
    
    // 设置约束的柔度
    // 参数：
    //   c - 新的柔度值
    void SetCompliance(float c)
    {
        compliance = c;
    }
    
    // 获取约束的柔度
    // 返回：柔度值
    float GetCompliance() const
    {
        return compliance;
    }
    
    // 约束的柔度参数
    float compliance; // 柔度（与刚度成反比）
};

#endif // CONSTRAINT_H

