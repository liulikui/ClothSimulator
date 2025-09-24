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
    LRAConstraint(Particle* particle, const dx::XMFLOAT3& attachmentPoint, float geodesicDistance, float compliance)
    {
        this->particle = particle;
        this->attachmentPoint = attachmentPoint;
        this->geodesicDistance = geodesicDistance;
        this->lambda = 0.0f;
        this->compliance = compliance;
        this->attachmentInitialPos = attachmentPoint;
    }

    // 计算约束偏差
    float ComputeConstraintValue() const override
    {
        if (particle->isStatic) return 0.0f;
        
        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR attachPos = dx::XMLoadFloat3(&attachmentPoint);
        dx::XMVECTOR diff = dx::XMVectorSubtract(pos, attachPos);
        float currentDistance = dx::XMVectorGetX(dx::XMVector3Length(diff));
        
        float constraintValue = currentDistance - this->geodesicDistance;
        if (constraintValue > 0.0f)
        {
            return constraintValue;
        }
        else
        {
            return 0.0f;
        }
    }

    // 计算约束梯度
    void ComputeGradient(std::vector<dx::XMFLOAT3>& gradients) const override
    {
        gradients.clear();
        if (particle->isStatic) return;
        
        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR attachPos = dx::XMLoadFloat3(&attachmentPoint);
        dx::XMVECTOR delta = dx::XMVectorSubtract(pos, attachPos);
        float currentDistance = dx::XMVectorGetX(dx::XMVector3Length(delta));
        
        if (currentDistance - this->geodesicDistance + 1e-9f < 0.0f)
        {
            gradients.push_back(dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
            return;
        }
        
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
        gradients.push_back(gradientFloat3);
    }

    // 获取受此约束影响的所有粒子
    std::vector<Particle*> GetParticles() override
    {
        std::vector<Particle*> result;
        result.push_back(particle);
        return result;
    }

    // 获取受此约束影响的所有粒子（const版本）
    std::vector<const Particle*> GetParticles() const override
    {
        std::vector<const Particle*> result;
        result.push_back(particle);
        return result;
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
};

#endif // LRA_CONSTRAINT_H