#ifndef DISTANCE_CONSTRAINT_H
#define DISTANCE_CONSTRAINT_H

#include "Constraint.h"
#include <DirectXMath.h>

#ifdef DEBUG_SOLVER
#include <string>
extern void logDebug(const std::string& message);
#endif//DEBUG_SOLVER

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 距离约束类，继承自约束基类
// 该约束保持两个粒子之间的距离为指定值
class DistanceConstraint : public Constraint 
{
public:
    // 构造函数
    // 参数：
    //   p1 - 第一个粒子的指针
    //   p2 - 第二个粒子的指针
    //   restLength - 约束的静止长度（两个粒子之间的目标距离）
    //   compliance - 约束的柔度（与刚度成反比）
    DistanceConstraint(Particle* p1, Particle* p2, float restLength, float compliance = 1e-6)
        : particle1(p1), particle2(p2), restLength(restLength)
    {
        this->compliance = compliance;
    }
    
    // 计算约束梯度
    // 参数：
    //   gradients - 存储每个受约束粒子的梯度向量的向量
    float ComputeConstraintAndGradient(dx::XMFLOAT3* gradients) const override
    {
        // 将XMFLOAT3转换为XMVECTOR进行计算
        dx::XMVECTOR pos1 = dx::XMLoadFloat3(&particle1->position);
        dx::XMVECTOR pos2 = dx::XMLoadFloat3(&particle2->position);
        
        // 计算两个粒子之间的向量差
        dx::XMVECTOR diff = dx::XMVectorSubtract(pos1, pos2);
        
        // 计算向量的长度（距离）
        float distance = dx::XMVectorGetX(dx::XMVector3Length(diff));
        
        if (distance > 0.0f)
        {
            // 归一化向量
            dx::XMVECTOR normalizedDiff = dx::XMVector3Normalize(diff);
            
            dx::XMFLOAT3 gradient1;
            dx::XMFLOAT3 gradient2;
            
            // 对第一个粒子的梯度
            dx::XMStoreFloat3(&gradient1, normalizedDiff);
            gradients[0] = gradient1;
            
            // 对第二个粒子的梯度（方向相反）
            dx::XMStoreFloat3(&gradient2, dx::XMVectorNegate(normalizedDiff));
            gradients[1] = gradient2;
        } 
        else 
        {
            // 避免除以零，当两个粒子重合时使用任意方向
            gradients[0] = dx::XMFLOAT3(1.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(-1.0f, 0.0f, 0.0f);
        }
		
        float C = distance - restLength;

#ifdef DEBUG_SOLVER
        char buffer[256];
        sprintf_s(buffer, "[DEBUG] C is large. C:%f,P1.coordW:%d,p1.coordH:%d,P1.x:%f, P1.y:%f,P1.z:%f,P2.coordW:%d,p2.coordH:%d,P2.x:%f, P2.y:%f,P2.z:%f"
            , C
            , particle1->coordW
            , particle1->coordH
            , particle1->position.x
            , particle1->position.y
            , particle1->position.z
            , particle2->coordW
            , particle2->coordH
            , particle2->position.x
            , particle2->position.y
            , particle2->position.z);
        logDebug(buffer);
#endif
        
		return C;
    }
    
    // 获取受此约束影响的所有粒子的数量
    // 返回：受约束影响的粒子数量
    virtual uint32_t GetParticlesCount() const override
    {
        return 2;
    }

    // 获取受此约束影响的所有粒子
    // 返回：受约束影响的粒子的数组
    virtual Particle** GetParticles() override
    {
        return &particle1;
    }

    // 获取受此约束影响的所有粒子
    // 返回：受约束影响的粒子的数组
    virtual const Particle** GetParticles() const override
    {
        return (const Particle**)(&particle1);
    }

    // 设置约束的静止长度
    // 参数：
    //   length - 新的静止长度
    void SetRestLength(float length)
    {
        restLength = length;
    }
    
    // 获取约束的静止长度
    // 返回：静止长度值
    float GetRestLength() const
    {
        return restLength;
    }
    
    // 获取约束类型
    virtual const char* GetConstraintType() const override
    {
        return "Distance";
    }

private:
    // 受约束的两个粒子
    Particle* particle1; // 第一个粒子
    Particle* particle2; // 第二个粒子
    
    // 约束的静止长度
    float restLength; // 两个粒子之间的目标距离
};

#endif // DISTANCE_CONSTRAINT_H
