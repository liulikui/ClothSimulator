#ifndef XPBD_SOLVER_H
#define XPBD_SOLVER_H

#include <vector>
#include <algorithm>  // 添加这个头文件以支持std::max和std::min
#include <unordered_map>  // 添加这个头文件以支持std::unordered_map，用于碰撞粒子速度反弹处理
#include <DirectXMath.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

// 使用inline函数来跟踪碰撞约束的应用次数，避免多重定义问题
inline int& getCollisionConstraintCount() {
    static int count = 0;
    return count;
}

#include "Particle.h"
#include "Constraint.h"

// 为了方便使用，创建一个命名空间别名
namespace dx = DirectX;

// XPBD (Extended Position Based Dynamics) 求解器
// 一种基于位置的物理模拟系统，特别适合处理约束
class XPBDSolver {
public:
    // 构造函数
    // 参数
    //   particles - 粒子数组引用
    //   constraints - 约束数组引用
    //   dt - 时间步长
    //   iterations - 约束求解的迭代次数
    XPBDSolver(std::vector<Particle>& particles, std::vector<Constraint*>& constraints,
              float dt = 1.0f/60.0f, int iterations = 50)  // 增加迭代次数到50次，提高高分辨率布料的稳定性
        : particles(particles), constraints(constraints), timeStep(dt), iterations(iterations) {
        // 初始化重力
        gravity = dx::XMFLOAT3(0.0f, -9.81f, 0.0f);
        
        // 调试输出已禁用
    }
    
    // 析构函数
    ~XPBDSolver() {
        // 调试输出已禁用，不需要关闭文件
    }
    
    // 调试信息输出（已禁用）
    void logDebugInfo(float timeStep, size_t iteration, size_t constraintIndex, const std::vector<Particle*>& constraintParticles, float C, float deltaLambda) {
        // 调试输出已禁用
    }
    
    // 设置时间步长
    // 参数
    //   dt - 新的时间步长
    void setTimeStep(float dt) {
        timeStep = dt;
    }
    
    // 设置约束求解的迭代次数
    // 参数
    //   its - 新的迭代次数
    void setIterations(int its) {
        iterations = its;
    }
    
    // 设置重力
    // 参数
    //   g - 新的重力向量
    void setGravity(const dx::XMFLOAT3& g) {
        gravity = g;
    }
    
    // 模拟一步
    // 执行一次完整的XPBD模拟步骤，包括预测、约束求解和位置校正
    void step(float deltaTime) {
        // 1. 预测粒子的位置，考虑外力
        predictPositions(deltaTime);
        
        // 2. 求解约束多次以获得更准确的结果
        for (int i = 0; i < iterations; ++i) {
            solveConstraints(deltaTime);
        }
        
        // 3. 更新速度和位置
        updateVelocities(deltaTime);
        
        // 4. 应用阻尼（可选）
        applyDamping();
    }
    
private:
    // 预测粒子的位置（考虑外力）
    void predictPositions(float deltaTime) {
        // 将重力转换为XMVECTOR进行计算
        dx::XMVECTOR gravityVector = dx::XMLoadFloat3(&gravity);
        
        for (auto& particle : particles) {
            if (!particle.isStatic) {
                // 保存当前位置作为旧位置
                particle.oldPosition = particle.position;
                
                // 应用重力：重力本身就是力，不需要再乘以质量
                particle.applyForce(gravity);
                
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
    void solveConstraints(float deltaTime) {
        for (auto& constraint : constraints) {
            // 计算约束值
            float C = constraint->computeConstraintValue();
            
            // 检查约束值是否有效
            if (isnan(C) || isinf(C)) {
                continue;
            }
            
            // 判断是否为碰撞约束（单粒子约束且C < 0表示粒子在球内）
            bool isCollisionConstraint = false;
            std::vector<Particle*> constraintParticles = constraint->getParticles();
            
            if (constraintParticles.size() == 1 && C < 0) {
                isCollisionConstraint = true;
                getCollisionConstraintCount()++;
                
                // 对于碰撞约束，我们只在粒子在球内时应用约束
                // C < 0表示粒子在球内，需要被推回球外
            } else if (constraintParticles.size() == 1) {
                // 碰撞约束但粒子在球外，不需要处理
                continue;
            } else if (std::abs(C) < 1e-9f) {
                // 其他约束，如果约束值很小，可以忽略
                continue;
            }
            
            // 检查粒子位置是否有效
            bool hasInvalidParticle = false;
            for (auto* particle : constraintParticles) {
                if (isnan(particle->position.x) || isnan(particle->position.y) || isnan(particle->position.z)) {
                    hasInvalidParticle = true;
                    break;
                }
            }
            if (hasInvalidParticle) {
                continue;
            }
            
            std::vector<dx::XMFLOAT3> gradients;
            constraint->computeGradient(gradients);
            
            // 计算分母项
            float sum = 0.0f;
            for (size_t i = 0; i < constraintParticles.size(); ++i) {
                if (!constraintParticles[i]->isStatic) {
                    // 将梯度转换为XMVECTOR进行点积计算
                    dx::XMVECTOR gradient = dx::XMLoadFloat3(&gradients[i]);
                    
                    // 计算梯度的点积
                    float dotProduct = dx::XMVectorGetX(dx::XMVector3Dot(gradient, gradient));
                    
                    sum += dotProduct * constraintParticles[i]->inverseMass;
                }
            }
            
            // 添加柔度项
            float alpha = constraint->getCompliance() / (deltaTime * deltaTime);
            sum += alpha;
            
            // 防止除零
            if (sum < 1e-9f) {
                sum = 1e-9f;
            }
            
            // 计算拉格朗日乘子增量 - 正确实现XPBD算法
            float deltaLambda = (-C - alpha * constraint->lambda) / sum;
            
            // 记录调试信息（已禁用）
            // static size_t constraintCounter = 0;
            // logDebugInfo(deltaTime, 0, constraintCounter++, constraintParticles, C, deltaLambda);
            
            // 对于碰撞约束，确保deltaLambda是非正的（只推粒子向外，不向内拉）
            if (isCollisionConstraint) {
                if (deltaLambda > 0.0f) {
                    deltaLambda = 0.0f;
                }
            }
            
            // 为提高稳定性，添加deltaLambda的上限限制
            float lambdaLimit = 100.0f; // 可以根据需要调整
            if (deltaLambda > lambdaLimit) {
                deltaLambda = lambdaLimit;
            } else if (deltaLambda < -lambdaLimit) {
                deltaLambda = -lambdaLimit;
            }
            
            // 更新约束的拉格朗日乘子
            constraint->lambda += deltaLambda;
            
            // 应用位置校正
            for (size_t i = 0; i < constraintParticles.size(); ++i) {
                if (!constraintParticles[i]->isStatic) {
                    // 将粒子的位置和梯度转换为XMVECTOR进行计算
                    dx::XMVECTOR pos = dx::XMLoadFloat3(&constraintParticles[i]->position);
                    dx::XMVECTOR gradient = dx::XMLoadFloat3(&gradients[i]);
                    
                    // 计算校正量
                    dx::XMVECTOR correction = dx::XMVectorScale(gradient, deltaLambda * constraintParticles[i]->inverseMass);
                    
                    // 为了提高稳定性，限制单次校正量的大小
                    float maxCorrection = 1.0f;  // 增加最大校正量，对于碰撞约束允许更大的校正
                    dx::XMVECTOR correctionLength = dx::XMVector3Length(correction);
                    if (dx::XMVectorGetX(correctionLength) > maxCorrection) {
                        correction = dx::XMVectorScale(correction, maxCorrection / dx::XMVectorGetX(correctionLength));
                    }
                    
                    // 应用校正
                    dx::XMVECTOR newPos = dx::XMVectorAdd(pos, correction);
                    
                    // 确保新位置有效
                    if (!dx::XMVector3IsNaN(newPos)) {
                        dx::XMStoreFloat3(&constraintParticles[i]->position, newPos);
                    }
                }
            }
        }
    }
    
    // 更新粒子的速度
    void updateVelocities(float deltaTime) {
        // 临时存储碰撞粒子的梯度信息，用于后续速度反弹
        std::unordered_map<Particle*, dx::XMVECTOR> collisionGradients;
        
        // 首先收集所有碰撞约束的梯度信息
        for (auto& constraint : constraints) {
            float C = constraint->computeConstraintValue();
            std::vector<Particle*> constraintParticles = constraint->getParticles();
            
            // 只处理碰撞约束（单粒子约束且C < 0表示粒子在球内）
            if (constraintParticles.size() == 1 && C < 0) {
                std::vector<dx::XMFLOAT3> gradients;
                constraint->computeGradient(gradients);
                
                if (!gradients.empty() && !constraintParticles[0]->isStatic) {
                    collisionGradients[constraintParticles[0]] = dx::XMLoadFloat3(&gradients[0]);
                }
            }
        }
        const float maxVelocity = 100.0f; // 降低最大速度限制，提高稳定性
        
        for (auto& particle : particles) {
            if (!particle.isStatic) {
                // 将粒子的位置和旧位置转换为XMVECTOR进行计算
                dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
                dx::XMVECTOR oldPos = dx::XMLoadFloat3(&particle.oldPosition);
                
                // 检查位置是否有效
                if (dx::XMVector3IsNaN(pos) || dx::XMVector3IsNaN(oldPos)) {
                    // 位置无效，保留当前速度不变
                    continue;
                }
                
                // 根据位置变化更新速度
                dx::XMVECTOR vel = dx::XMVectorScale(dx::XMVectorSubtract(pos, oldPos), 1.0f / deltaTime);
                
                // 检查速度是否有效
                if (dx::XMVector3IsNaN(vel)) {
                    // 速度无效，设置为零
                    vel = dx::XMVectorZero();
                } else {
                    // 限制速度大小，防止速度过大
                    float speed = dx::XMVectorGetX(dx::XMVector3Length(vel));
                    if (speed > maxVelocity) {
                        vel = dx::XMVectorScale(vel, maxVelocity / speed);
                    }
                }
                
                // 检查是否有碰撞发生，如果有，应用速度反弹
                auto it = collisionGradients.find(&particle);
                if (it != collisionGradients.end()) {
                    dx::XMVECTOR normal = it->second;
                    
                    // 计算速度在法线上的投影
                    float velocityAlongNormal = dx::XMVectorGetX(dx::XMVector3Dot(vel, normal));
                    
                    // 如果粒子向球体内部移动（法向速度分量为负），则应用反弹
                    if (velocityAlongNormal < 0) {
                        float restitution = 0.4f;  // 弹性系数，控制反弹强度
                        dx::XMVECTOR reflection = dx::XMVectorScale(normal, (1.0f + restitution) * velocityAlongNormal);
                        vel = dx::XMVectorSubtract(vel, reflection);
                    }
                }
                
                // 将结果转换回XMFLOAT3
                dx::XMStoreFloat3(&particle.velocity, vel);
                
                // 清除力，为下一帧做准备
                particle.force = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
        }
    }
    
    // 应用阻尼
    void applyDamping() {
        const float velocityThreshold = 2.0f;
        const float defaultDampingFactor = 0.97f;
        const float highDampingFactor = 0.85f;
        
        for (auto& particle : particles) {
            if (!particle.isStatic) {
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
    std::vector<Particle>& particles; // 粒子数组的引用
    std::vector<Constraint*>& constraints; // 约束数组的引用
    float timeStep; // 时间步长
    int iterations; // 约束求解的迭代次数
    dx::XMFLOAT3 gravity; // 重力向量
    // std::ofstream debugFile; // 调试输出文件流（已禁用）
};

#endif // XPBD_SOLVER_H