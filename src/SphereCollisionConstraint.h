#pragma once

#include <DirectXMath.h>
#include "Constraint.h"
#include "Particle.h"

namespace dx = DirectX;

class SphereCollisionConstraint : public Constraint
{
public:
    SphereCollisionConstraint(Particle* p, const dx::XMFLOAT3& center, float radius, float compliance, float damping)
        : Constraint(compliance, damping)
        , m_particle(p)
        , m_sphereCenter(center)
        , m_sphereRadius(radius)
    {
    }

    float ComputeConstraintAndGradient(dx::XMFLOAT3* gradients) const override
    {
        if (m_particle->isStatic)
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            return 0.0f;
        }
        else
        {
            dx::XMVECTOR pos = dx::XMLoadFloat3(&m_particle->position);
            dx::XMVECTOR center = dx::XMLoadFloat3(&m_sphereCenter);
            dx::XMVECTOR toCenter = dx::XMVectorSubtract(pos, center);
            float distance = dx::XMVectorGetX(dx::XMVector3Length(toCenter));

            if (distance > m_sphereRadius)
            {
                gradients[0] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);

                return 0.0f;
            }
            else
            {
                if (distance > 1e-6f)
                {
                    dx::XMVECTOR gradient = dx::XMVectorScale(toCenter, 1.0f / distance);
                    dx::XMFLOAT3 gradientFloat3;
                    dx::XMStoreFloat3(&gradientFloat3, gradient);

                    gradients[0] = gradientFloat3;

                    return distance - m_sphereRadius;
                }
                else
                {
                    gradients[0] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);

                    return 0.0f;
                }
            }
        }
    }

    virtual uint32_t GetParticlesCount() const override
    {
        return 1;
    }

    virtual Particle** GetParticles()
    {
        return &m_particle;
    }

    virtual const Particle** GetParticles() const override
    {
        return (const Particle**)(&m_particle);
    }

    virtual const char* GetConstraintType() const override
    {
        return "SphereCollision";
    }

private:
    Particle* m_particle;
    dx::XMFLOAT3 m_sphereCenter;
    float m_sphereRadius;
};