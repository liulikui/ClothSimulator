#include "XPBDSolver.h"
#include "Cloth.h"
#include "AutoMem.h"

#ifdef DEBUG_SOLVER
extern void logDebug(const std::string& message);
#endif//DEBUG_SOLVER

void XPBDSolver::BeginStep()
{
    for (auto& particle : m_cloth->m_particles)
    {
        particle.positionInitial = particle.position;
    }
}

void XPBDSolver::PredictPositions(float deltaTime)
{
    for (auto& particle : m_cloth->m_particles)
    {
        if (!particle.isStatic)
        {
            // 保存当前位置作为旧位置
            particle.oldPosition = particle.position;

            // 应用重力
            particle.ApplyForce(m_cloth->m_gravity);

            // 将粒子的位置和速度转换为XMVECTOR进行计算
            dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
            dx::XMVECTOR vel = dx::XMLoadFloat3(&particle.velocity);
            dx::XMVECTOR force = dx::XMLoadFloat3(&particle.force);

            // 预测新位置（使用显式欧拉积分）
            pos = dx::XMVectorAdd(pos, dx::XMVectorScale(vel, deltaTime));
            pos = dx::XMVectorAdd(pos, dx::XMVectorScale(dx::XMVectorScale(force, particle.inverseMass), 0.5f * deltaTime * deltaTime));

            // 保存预测位置
            dx::XMStoreFloat3(&particle.predPosition, pos);

            // 初始位置设置为预测位置
            dx::XMStoreFloat3(&particle.position, pos);
        }
    }
}

void XPBDSolver::Step(float deltaTime)
{
    BeginStep();

    float subDeltaTime = deltaTime / m_cloth->m_subIteratorCount;

    for (int i = 0; i < m_cloth->m_subIteratorCount; ++i)
    {
        // 1. 预测粒子的位置，考虑外力
        PredictPositions(subDeltaTime);

        // 2. 求解约束多次以获得更准确的结果
        for (int i = 0; i < m_cloth->m_iteratorCount; ++i)
        {
            SolveConstraints(subDeltaTime);
        }

        // 3. 更新速度和位置
        UpdateVelocities(subDeltaTime);
    }

    EndStep(deltaTime);
}

void XPBDSolver::SolveConstraints(float deltaTime)
{
    // 处理距离约束
    for (auto& constraint : m_cloth->m_distanceConstraints)
    {
        SolveConstraint(&constraint, deltaTime);
    }

    // 处理弯曲约束
    for (auto& constraint : m_cloth->m_dihedralBendingConstraints)
    {
        SolveConstraint(&constraint, deltaTime);
    }

    // 处理LRA约束
    for (auto& constraint : m_cloth->m_lraConstraints)
    {
        SolveConstraint(&constraint, deltaTime);
    }

    // 处理碰撞约束
    for (auto& constraint : m_cloth->m_CollisionConstraints)
    {
        SolveConstraint(constraint, deltaTime);
    }
}

void XPBDSolver::SolveConstraint(Constraint* constraint, float deltaTime)
{
    uint32_t particleCount = constraint->GetParticlesCount();

    if (particleCount == 0)
    {
        return;
    }

    Particle** constraintParticles = constraint->GetParticles();

    TAutoMem<dx::XMFLOAT3, 16> gradientsBuffer(particleCount);
    dx::XMFLOAT3* gradients = gradientsBuffer.GetBuffer();

    // 计算约束值和梯度
    float C = constraint->ComputeConstraintAndGradient(gradients);

#ifdef DEBUG_SOLVER
    // 检查约束值是否有效
    if (isnan(C) || isinf(C))
    {
        logDebug("[DEBUG] InvalidConstraintValue)");
        return;
    }
#endif//DEBUG_SOLVER

    if (std::abs(C) < 1e-9f)
    {
        // 如果约束值很小，可以忽略
        return;
    }

    // 计算分母项
    double sum = 0.0;
    double delta_pos_total = 0.0;

    for (uint32_t i = 0; i < particleCount; ++i)
    {
        Particle* particle = constraintParticles[i];

        if (!particle->isStatic)
        {
            // 将梯度转换为XMVECTOR进行点积计算
            dx::XMVECTOR gradient = dx::XMLoadFloat3(&gradients[i]);

            // 计算梯度的点积
            float dotProduct = dx::XMVectorGetX(dx::XMVector3Dot(gradient, gradient));
            sum += dotProduct * particle->inverseMass;

            // 计算梯度与delta_pos的点积
            dx::XMVECTOR delta_pos = dx::XMVectorSubtract(dx::XMLoadFloat3(&particle->position), 
                dx::XMLoadFloat3(&particle->predPosition));
            delta_pos_total += dx::XMVectorGetX(dx::XMVector3Dot(gradient, delta_pos));
        }
    }

    // 添加柔度项
    double alpha_tilde = constraint->GetCompliance() / ((double)deltaTime * (double)deltaTime);

    if (alpha_tilde > 1e6f)
    {
        alpha_tilde = 1e6f;
    }

    double gamma = constraint->GetDamping() * (double)deltaTime;

    sum = (1 + gamma) * sum + alpha_tilde;

    // 防止除零
    if (sum < 1e-9f)
    {
        sum = 1e-9f;
    }

    // 计算拉格朗日乘子增量
    double deltaLambda = (double(-C - alpha_tilde * constraint->GetLambda() - gamma * delta_pos_total) / sum);

#ifdef DEBUG_SOLVER
    // 检查约束值是否有效
    if (isnan(deltaLambda) || isinf(deltaLambda))
    {
        char buffer[256];
        sprintf_s(buffer, "[DEBUG] deltaLambda is invalid:%f C:%f alpha_tilde:%f Lambda:%f gamma:%f delta_pos_total:%f"
            , deltaLambda
            , C
            , alpha_tilde
            , constraint->GetLambda()
            , gamma
            , delta_pos_total);
        logDebug(buffer);
    }
#endif//DEBUG_SOLVER

    // 应用位置校正
    for (uint32_t i = 0; i < particleCount; ++i)
    {
        Particle* particle = constraintParticles[i];

        if (!particle->isStatic)
        {
            // 将粒子的位置和梯度转换为XMVECTOR进行计算
            dx::XMVECTOR pos = dx::XMLoadFloat3(&particle->position);
            dx::XMVECTOR gradient = dx::XMLoadFloat3(&gradients[i]);

            // 计算校正量
            dx::XMVECTOR correction = dx::XMVectorScale(gradient, deltaLambda * particle->inverseMass);

#ifdef DEBUG_SOLVER
            dx::XMVECTOR correctionLength = dx::XMVector3Length(correction);
            char buffer[256];
            sprintf_s(buffer, "[DEBUG] constraintType:%s deltaTime:%f coordW:%d coordH:%d C:%f compliance:%f alpha_tilde:%f lambda:%f deltaLambda:%f gamma:%f delta_pos_total:%f correctionLength:%f"
                , constraint->GetConstraintType()
                , deltaTime
                , particle->coordW
                , particle->coordH
                , C
                , constraint->GetCompliance()
                , alpha_tilde
                , constraint->GetLambda()
                , deltaLambda
                , gamma
                , delta_pos_total
                , dx::XMVectorGetX(correctionLength));
            logDebug(buffer);
#endif//DEBUG_SOLVER

            // 应用校正
            dx::XMVECTOR newPos = dx::XMVectorAdd(pos, correction);

            // 确保新位置有效
            if (!dx::XMVector3IsNaN(newPos))
            {
                dx::XMStoreFloat3(&particle->position, newPos);
            }
        }
    }

    // 更新约束的拉格朗日乘子
    constraint->SetLambda(constraint->GetLambda() + deltaLambda);

#ifdef DEBUG_SOLVER
    constraint->Check();
#endif//DEBUG_SOLVER
}

void XPBDSolver::UpdateVelocities(float deltaTime)
{
    for (auto& particle : m_cloth->m_particles)
    {
        if (!particle.isStatic)
        {
            // 将粒子的位置和旧位置转换为XMVECTOR进行计算
            dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
            dx::XMVECTOR oldPos = dx::XMLoadFloat3(&particle.oldPosition);

            // 根据位置变化更新速度
            dx::XMVECTOR vel = dx::XMVectorScale(dx::XMVectorSubtract(pos, oldPos), 1.0f / deltaTime);

            // 将结果转换回XMFLOAT3
            dx::XMStoreFloat3(&particle.velocity, vel);

			// 重置力
            particle.ResetForce();
        }
    }
}

void XPBDSolver::EndStep(float deltaTime)
{
    for (auto& particle : m_cloth->m_particles)
    {
        if (!particle.isStatic)
        {
            dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
            dx::XMVECTOR posInitial = dx::XMLoadFloat3(&particle.positionInitial);

            // 根据位置变化更新速度
            dx::XMVECTOR velFinal = dx::XMVectorScale(dx::XMVectorSubtract(pos, posInitial), 1.0f / deltaTime);

            dx::XMStoreFloat3(&particle.velocity, velFinal);
        }
    }
}