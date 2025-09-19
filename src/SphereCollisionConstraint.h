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
        compliance = 1e-7f;
    }
    
    SphereCollisionConstraint(Particle* p, const dx::XMFLOAT3& center, float radius, float customCompliance)
    {
        particle = p;
        sphereCenter = center;
        sphereRadius = radius;
        compliance = customCompliance;
    }

    float computeConstraintValue() const override {
        if (particle->isStatic) return 0.0f;
        
        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR center = dx::XMLoadFloat3(&sphereCenter);
        dx::XMVECTOR toCenter = dx::XMVectorSubtract(pos, center);
        float distance = dx::XMVectorGetX(dx::XMVector3Length(toCenter));

        return distance - sphereRadius;
    }

    void computeGradient(std::vector<dx::XMFLOAT3>& gradients) const override
    {
        gradients.clear();
        if (particle->isStatic) return;

        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
        dx::XMVECTOR center = dx::XMLoadFloat3(&sphereCenter);
        dx::XMVECTOR toCenter = dx::XMVectorSubtract(pos, center);
        float distance = dx::XMVectorGetX(dx::XMVector3Length(toCenter));

        if (distance > 1e-9f)
        {
            dx::XMVECTOR gradient = dx::XMVectorScale(toCenter, 1.0f / distance);
            dx::XMFLOAT3 gradientFloat3;
            dx::XMStoreFloat3(&gradientFloat3, gradient);
            gradients.push_back(gradientFloat3);
        }
        else
        {
            gradients.push_back(dx::XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
    }

    std::vector<Particle*> getParticles() override
    {
        std::vector<Particle*> result;
        result.push_back(particle);
        return result;
    }

    std::vector<const Particle*> getParticles() const override
    {
        std::vector<const Particle*> result;
        result.push_back(particle);
        return result;
    }

private:
    Particle* particle;
    dx::XMFLOAT3 sphereCenter;
    float sphereRadius;
};