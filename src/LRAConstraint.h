#ifndef LRA_CONSTRAINT_H
#define LRA_CONSTRAINT_H

#include <DirectXMath.h>
#include <algorithm>
#include "Constraint.h"
#include "Particle.h"

namespace dx = DirectX;

// LRA约束类，继承自约束基类
// 该约束限制布料粒子与固定附着点之间的测地线距离
class LRAConstraint : public Constraint
{
public:
    // 构造函数
    LRAConstraint(Particle* particle, const dx::XMFLOAT3& attachmentPoint, float geodesicDistance, float compliance, float maxStretch)
    {
        this->particle = particle;
        this->attachmentPoint = attachmentPoint;
        this->geodesicDistance = geodesicDistance;
        this->lambda = 0.0f;
        this->compliance = compliance;
        this->attachmentInitialPos = attachmentPoint;
        this->maxStretch = maxStretch;
    }

    // 计算约束偏差和约束梯度
    float ComputeConstraintAndGradient(dx::XMFLOAT3* gradients) const override
    {
        if (particle->isStatic)
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            return 0.0f;
        }

        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR attachPos = dx::XMLoadFloat3(&attachmentPoint);
        dx::XMVECTOR delta = dx::XMVectorSubtract(pos, attachPos);
        float currentDistance = dx::XMVectorGetX(dx::XMVector3Length(delta));
        
        float constraintValue = currentDistance - geodesicDistance * (1 + maxStretch);

        if (constraintValue > 0.0f)
        {
			dx::XMVECTOR gradient;
            if (currentDistance > 1e-9f)
            {
                gradient = dx::XMVectorScale(delta, 1.0f / currentDistance);
            }
            else
            {
                gradient = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            }

            dx::XMFLOAT3 gradientFloat3;
            dx::XMStoreFloat3(&gradientFloat3, gradient);
            gradients[0] = gradientFloat3;
			
			return constraintValue;
 		}
        else
        {
			gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            return 0.0f;
        }
    }

    // 获取受此约束影响的所有粒子的数量
    // 返回：受约束影响的粒子数量
    virtual uint32_t GetParticlesCount() const override
    {
        return 1;
    }

    // 获取受此约束影响的所有粒子
    // 返回：受约束影响的粒子的数组
    virtual Particle** GetParticles()
    {
        return &particle;
    }

    // 获取受此约束影响的所有粒子
     // 返回：受约束影响的粒子的数组
    virtual const Particle** GetParticles() const override
    {
        return (const Particle**)(&particle);
    }

    // 更新附着点位置
    void UpdateAttachmentPoint(const dx::XMFLOAT3& newPosition)
    {
        this->attachmentPoint = newPosition;
    }

    // 获取当前附着点位置
    const dx::XMFLOAT3& GetAttachmentPoint() const
    {
        return this->attachmentPoint;
    }

    // 获取初始附着点位置
    const dx::XMFLOAT3& GetInitialAttachmentPoint() const
    {
        return this->attachmentInitialPos;
    }

    // 获取约束类型
    virtual const char* GetConstraintType() const override
    {
        return "LRA";
    }

private:
    Particle* particle;
    dx::XMFLOAT3 attachmentPoint;
    dx::XMFLOAT3 attachmentInitialPos;
    float geodesicDistance;
    float maxStretch;
};

#endif // LRA_CONSTRAINT_H