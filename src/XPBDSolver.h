#ifndef XPBD_SOLVER_H
#define XPBD_SOLVER_H

#include <vector>
#include <DirectXMath.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include "Particle.h"
#include "Constraint.h"

// 为了方便使用，创建一个命名空间别名
namespace dx = DirectX;

extern void logDebug(const std::string& message);

// XPBD (Extended Position Based Dynamics) 求解器
// 一种基于位置的物理模拟系统，特别适合处理约束
class XPBDSolver
{
public:
    // 构造函数
    // 参数
    //   particles - 粒子数组引用
    //   constraints - 约束数组引用
    //   iterations - 约束求解的迭代次数
    XPBDSolver(std::vector<Particle>& particles, std::vector<Constraint*>& constraints, int iterations)
        : m_particles(particles)
        , m_constraints(constraints)
        , m_iterations(iterations)
        , m_timeStep(0.1f)
    {
        // 初始化重力
        m_gravity = dx::XMFLOAT3(0.0f, -9.81f, 0.0f);
    }
    
    // 析构函数
    ~XPBDSolver()
    {
    }
    
    // 设置时间步长
    // 参数
    //   dt - 新的时间步长
    void SetTimeStep(float dt)
    {
        m_timeStep = dt;
    }
    
    // 设置约束求解的迭代次数
    // 参数
    //   its - 新的迭代次数
    void SetIterations(int its)
    {
        m_iterations = its;
    }
    
    // 设置重力
    // 参数
    //   g - 新的重力向量
    void SetGravity(const dx::XMFLOAT3& g)
    {
        m_gravity = g;
    }
    
    // 模拟一步
    // 执行一次完整的XPBD模拟步骤，包括预测、约束求解和位置校正
    void Step(float deltaTime)
    {
        // 1. 预测粒子的位置，考虑外力
        PredictPositions(deltaTime);
        
        // 2. 求解约束多次以获得更准确的结果
        for (int i = 0; i < m_iterations; ++i)
        {
            SolveConstraints(deltaTime);
        }
        
        // 3. 更新速度和位置
        UpdateVelocities(deltaTime);
        
        // 4. 应用阻尼（可选）
        ApplyDamping();
    }
    
private:
    // 预测粒子的位置（考虑外力）
    void PredictPositions(float deltaTime)
    {
        // 将重力转换为XMVECTOR进行计算
        dx::XMVECTOR gravityVector = dx::XMLoadFloat3(&m_gravity);
        
        for (auto& particle : m_particles)
        {
            if (!particle.isStatic)
            {
                // 保存当前位置作为旧位置
                particle.oldPosition = particle.position;
                
                // 应用重力：重力本身就是力，不需要再乘以质量
                particle.ApplyForce(m_gravity);
                
                // 将粒子的位置和速度转换为XMVECTOR进行计算
                dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
                dx::XMVECTOR vel = dx::XMLoadFloat3(&particle.velocity);
                dx::XMVECTOR force = dx::XMLoadFloat3(&particle.force);
                
                // 预测新位置（使用显式欧拉积分）
                pos = dx::XMVectorAdd(pos, dx::XMVectorScale(vel, deltaTime));
                pos = dx::XMVectorAdd(pos, dx::XMVectorScale(dx::XMVectorScale(force, particle.inverseMass), 0.5f * deltaTime * deltaTime));
                
                // 将结果转换回XMFLOAT3
                dx::XMStoreFloat3(&particle.position, pos);
            }
        }
    }
    
    // 求解所有约束
    void SolveConstraints(float deltaTime)
    {
        for (auto& constraint : m_constraints)
        {
            // 计算约束值
            float C = constraint->ComputeConstraintValue();
            
            // 检查约束值是否有效
            if (isnan(C) || isinf(C))
            {
                continue;
            }

            if (std::abs(C) < 1e-9f)
            {
                // 其他约束，如果约束值很小，可以忽略
                continue;
            }
            
            std::vector<Particle*> constraintParticles = constraint->GetParticles();

            // 检查粒子位置是否有效
            bool hasInvalidParticle = false;
            for (auto* particle : constraintParticles)
            {
                if (isnan(particle->position.x) || isnan(particle->position.y) || isnan(particle->position.z))
                {
                    hasInvalidParticle = true;
                    break;
                }
            }
            if (hasInvalidParticle)
            {
                continue;
            }
            
            std::vector<dx::XMFLOAT3> gradients;
            constraint->ComputeGradient(gradients);
            
            // 计算分母项
            float sum = 0.0f;
            for (size_t i = 0; i < constraintParticles.size(); ++i)
            {
                if (!constraintParticles[i]->isStatic)
                {
                    // 将梯度转换为XMVECTOR进行点积计算
                    dx::XMVECTOR gradient = dx::XMLoadFloat3(&gradients[i]);
                    
                    // 计算梯度的点积
                    float dotProduct = dx::XMVectorGetX(dx::XMVector3Dot(gradient, gradient));
                    
                    sum += dotProduct * constraintParticles[i]->inverseMass;
                }
            }
            
            // 添加柔度项
            float alpha = constraint->GetCompliance() / (deltaTime * deltaTime);
            sum += alpha;
            
            // 防止除零
            if (sum < 1e-9f)
            {
                sum = 1e-9f;
            }
            
            // 计算拉格朗日乘子增量
            float deltaLambda = (-C - alpha * constraint->lambda) / sum;

            // 更新约束的拉格朗日乘子
            constraint->lambda += deltaLambda;
            
            // 应用位置校正
            for (size_t i = 0; i < constraintParticles.size(); ++i)
            {
                if (!constraintParticles[i]->isStatic)
                {
                    // 将粒子的位置和梯度转换为XMVECTOR进行计算
                    dx::XMVECTOR pos = dx::XMLoadFloat3(&constraintParticles[i]->position);
                    dx::XMVECTOR gradient = dx::XMLoadFloat3(&gradients[i]);
                    
                    // 计算校正量
                    dx::XMVECTOR correction = dx::XMVectorScale(gradient, deltaLambda * constraintParticles[i]->inverseMass);

#ifdef DEBUG_SOLVER
                    dx::XMVECTOR correctionLength = dx::XMVector3Length(correction);
                    char buffer[256];
                    sprintf_s(buffer, "[DEBUG] constraintType:%s deltaTime:%f coordX:%d,coordY:%d alpha:%f deltaLambda:%f correctionLength:%f"
                        , constraint->GetConstraintType()
                        , deltaTime
                        , constraintParticles[i]->coordX
                        , constraintParticles[i]->coordY
                        , alpha
                        , deltaLambda
                        , dx::XMVectorGetX(correctionLength));
                    logDebug(buffer);
#endif//DEBUG_SOLVER
                    // 应用校正
                    dx::XMVECTOR newPos = dx::XMVectorAdd(pos, correction);
                    
                    // 确保新位置有效
                    if (!dx::XMVector3IsNaN(newPos))
                    {
                        dx::XMStoreFloat3(&constraintParticles[i]->position, newPos);
                    }
                }
            }
        }
    }
    
    // 更新粒子的速度
    void UpdateVelocities(float deltaTime)
    {
        for (auto& particle : m_particles)
        {
            if (!particle.isStatic)
            {
                // 将粒子的位置和旧位置转换为XMVECTOR进行计算
                dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
                dx::XMVECTOR oldPos = dx::XMLoadFloat3(&particle.oldPosition);
                
                // 检查位置是否有效
                if (dx::XMVector3IsNaN(pos) || dx::XMVector3IsNaN(oldPos))
                {
                    // 位置无效，保留当前速度不变
                    continue;
                }
                
                // 根据位置变化更新速度
                dx::XMVECTOR vel = dx::XMVectorScale(dx::XMVectorSubtract(pos, oldPos), 1.0f / deltaTime);
                
                // 检查速度是否有效
                if (dx::XMVector3IsNaN(vel))
                {
                    // 速度无效，设置为零
                    vel = dx::XMVectorZero();
                } 
                
                // 将结果转换回XMFLOAT3
                dx::XMStoreFloat3(&particle.velocity, vel);
                
                // 清除力，为下一帧做准备
                particle.force = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
        }
    }
    
    // 应用阻尼
    void ApplyDamping()
    {
        const float velocityThreshold = 2.0f;
        const float defaultDampingFactor = 0.97f;
        const float highDampingFactor = 0.85f;
        
        for (auto& particle : m_particles)
        {
            if (!particle.isStatic)
            {
                // 将粒子的速度转换为XMVECTOR进行计算
                dx::XMVECTOR vel = dx::XMLoadFloat3(&particle.velocity);
                
                // 计算速度大小
                float speed = dx::XMVectorGetX(dx::XMVector3Length(vel));
                
                // 根据速度大小选择阻尼系数，实现自适应阻尼
                float dampingFactor = (speed > velocityThreshold) ? highDampingFactor : defaultDampingFactor;
                
                // 应用阻尼
                vel = dx::XMVectorScale(vel, dampingFactor);
                
                // 将结果转换回XMFLOAT3
                dx::XMStoreFloat3(&particle.velocity, vel);
            }
        }
    }
    
    // 成员变量
    std::vector<Particle>& m_particles; // 粒子数组的引用
    std::vector<Constraint*>& m_constraints; // 约束数组的引用
    float m_timeStep; // 时间步长
    int m_iterations; // 约束求解的迭代次数
    dx::XMFLOAT3 m_gravity; // 重力向量
};

#endif // XPBD_SOLVER_H
 
