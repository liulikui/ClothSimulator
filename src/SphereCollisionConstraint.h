#pragma once

#include <DirectXMath.h>
#include "Constraint.h"
#include "Particle.h"

namespace dx = DirectX;

class SphereCollisionConstraint : public Constraint
{
public:
    SphereCollisionConstraint(Particle* p, const dx::XMFLOAT3& center, float radius)
    {
        particle = p;
        sphereCenter = center;
        sphereRadius = radius;
        compliance = 1e-2f;
    }
    
    SphereCollisionConstraint(Particle* p, const dx::XMFLOAT3& center, float radius, float customCompliance)
    {
        particle = p;
        sphereCenter = center;
        sphereRadius = radius;
        compliance = customCompliance;
    }

    float ComputeConstraintValue() const override
    {
        if (particle->isStatic) return 0.0f;
        
        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR center = dx::XMLoadFloat3(&sphereCenter);
        dx::XMVECTOR toCenter = dx::XMVectorSubtract(pos, center);
        float distance = dx::XMVectorGetX(dx::XMVector3Length(toCenter));

        if (distance > sphereRadius)
        {
            return 0.0f;
        }
        else
        {
            return distance - sphereRadius;
        }
    }

    void ComputeGradient(dx::XMFLOAT3* gradients) const override
    {
        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR center = dx::XMLoadFloat3(&sphereCenter);
        dx::XMVECTOR toCenter = dx::XMVectorSubtract(pos, center);
        float distance = dx::XMVectorGetX(dx::XMVector3Length(toCenter));

        if (distance > 1e-9f)
        {
            dx::XMVECTOR gradient = dx::XMVectorScale(toCenter, 1.0f / distance);
            dx::XMFLOAT3 gradientFloat3;
            dx::XMStoreFloat3(&gradientFloat3, gradient);
            gradients[0] = gradientFloat3;
        }
        else
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
        }
    }

    virtual uint32_t GetParticlesCount() const override
    {
        return 1;
    }

    virtual Particle** GetParticles()
    {
        return &particle;
    }

    virtual const Particle** GetParticles() const override
    {
        return (const Particle**)(&particle);
    }

    virtual const char* GetConstraintType() const override
    {
        return "SphereCollision";
    }

private:
    Particle* particle;
    dx::XMFLOAT3 sphereCenter;
    float sphereRadius;
};