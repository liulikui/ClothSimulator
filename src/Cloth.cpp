#include "Cloth.h"
#include <iostream>
#include <DirectXMath.h>
#include <algorithm>
#include "SphereCollisionConstraint.h"

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 声明外部函数
extern int& getCollisionConstraintCount();

// 构造函数：创建一个布料对象
// 参数：
//   position - 布料左下角的初始位置
//   width - 布料宽度方向的粒子数
//   height - 布料高度方向的粒子数
//   size - 布料的实际物理尺寸（以米为单位）
//   mass - 每个粒子的质量
Cloth::Cloth(const dx::XMFLOAT3& position, int width, int height, float size, float mass)
    : width(width), height(height), solver(particles, constraints), useXPBDCollision(true) {
    // 创建粒子
    createParticles(position, size, mass);
    
    // 创建布料的约束
    createConstraints(size);
    
    // 设置重力为标准地球重力
    gravity = dx::XMFLOAT3(0.0f, -9.8f, 0.0f);
    solver.setGravity(gravity);
    
    // 设置求解器参数（增加迭代次数以提高约束求解质量）
    solver.setIterations(80); // 显著增加迭代次数以提高高分辨率布料和碰撞的稳定性
    
    // 初始化球体碰撞约束（一次性创建，避免每帧重建）
    // 默认球体参数：中心在(0,0,0)，半径为2.0
    dx::XMFLOAT3 sphereCenter(0.0f, 0.0f, 0.0f);
    float sphereRadius = 2.0f;
    initializeSphereCollisionConstraints(sphereCenter, sphereRadius);
}

// 析构函数
Cloth::~Cloth() {
    // 清除球体碰撞约束
    clearSphereCollisionConstraints();
    
    // 清除距离约束
    for (auto& constraint : distanceConstraints) {
        // 注意：distanceConstraints中的对象是直接存储的，不需要delete
    }
    
    // 清除约束指针数组中的其他约束（如果有）
    constraints.clear();
}

// 初始化球体碰撞约束（一次性创建，避免每帧重建）
void Cloth::initializeSphereCollisionConstraints(const dx::XMFLOAT3& sphereCenter, float sphereRadius) {
    // 为每个非静态粒子创建一个球体碰撞约束，并设置更小的柔度值以提高碰撞刚性
    for (auto& particle : particles) {
        if (!particle.isStatic) {
            SphereCollisionConstraint* constraint = new SphereCollisionConstraint(
                &particle, sphereCenter, sphereRadius, 1e-9f); // 减小柔度值以增加刚性
            sphereConstraints.push_back(constraint);
        }
    }
    
    // 如果启用了XPBD碰撞，直接将球体碰撞约束添加到总约束列表中（只做一次）
    if (useXPBDCollision) {
        for (auto constraint : sphereConstraints) {
            constraints.push_back(reinterpret_cast<Constraint*>(constraint));
        }
    }
}

// 更新布料状态
void Cloth::update(float deltaTime) {
    // 重置碰撞约束计数
    getCollisionConstraintCount() = 0;
    
    // 添加重力
    solver.setGravity(gravity);
    
    // 设置球体参数
    dx::XMFLOAT3 sphereCenter(0.0f, 0.0f, 0.0f); // 与Main.cpp中的注释一致，使布料中心对准球体(0,0,0)
    float sphereRadius = 2.0f;
    
    // 更新球体碰撞状态已经在初始化时完成，这里只需要根据useXPBDCollision的值判断
    // 注意：约束的添加/移除操作现在只在initializeSphereCollisionConstraints和setUseXPBDCollision中执行
    // 这样可以避免每帧执行昂贵的约束列表操作，提高性能
    
    
    // 使用XPBD求解器更新布料状态
    solver.step(deltaTime);
    
    // 无论是否使用XPBD碰撞约束，都执行传统碰撞检测作为后备机制
    // 这可以确保即使XPBD碰撞约束没有正常工作，传统碰撞检测也能防止布料穿透球体
    checkSphereCollision(sphereCenter, sphereRadius);
    
    // 只保留碰撞约束计数逻辑，移除调试输出以提高性能
}

// 检查布料粒子与球体的碰撞（传统方法）
// 参数：
//   sphereCenter - 球体中心位置
//   sphereRadius - 球体半径
void Cloth::checkSphereCollision(const dx::XMFLOAT3& sphereCenter, float sphereRadius) {
    // 将球心转换为XMVECTOR进行计算
    dx::XMVECTOR center = dx::XMLoadFloat3(&sphereCenter);
    
    for (auto& particle : particles) {
        if (particle.isStatic) {
            continue; // 静态粒子不受碰撞影响
        }
        
        // 计算粒子到球心的向量
        dx::XMVECTOR pos = dx::XMLoadFloat3(&particle.position);
        dx::XMVECTOR direction = dx::XMVectorSubtract(pos, center);
        float distance = dx::XMVectorGetX(dx::XMVector3Length(direction));
        
        // 如果粒子在球体内部，将其推回球表面
        if (distance < sphereRadius) {
            // 计算粒子在球体表面上的新位置
            dx::XMVECTOR normal = dx::XMVector3Normalize(direction);
            dx::XMVECTOR newPos = dx::XMVectorAdd(center, dx::XMVectorScale(normal, sphereRadius));
            
            // 将结果转换回XMFLOAT3
            dx::XMStoreFloat3(&particle.position, newPos);
        }
    }
}

// 添加基于XPBD约束的球体碰撞检测
void Cloth::addSphereCollisionConstraint(const dx::XMFLOAT3& sphereCenter, float sphereRadius) {
    for (auto& particle : particles) {
        if (!particle.isStatic) {
            // 为每个非静态粒子创建一个球体碰撞约束
            SphereCollisionConstraint* constraint = new SphereCollisionConstraint(&particle, sphereCenter, sphereRadius);
            sphereConstraints.push_back(constraint);
            
            // 将约束添加到求解器的约束列表中
            constraints.push_back(constraint);
        }
    }
}

// 清除所有球体碰撞约束
void Cloth::clearSphereCollisionConstraints() {
    // 从总约束列表中移除球体碰撞约束
    auto it = std::remove_if(constraints.begin(), constraints.end(), [this](Constraint* constraint) {
        return std::find(sphereConstraints.begin(), sphereConstraints.end(), constraint) != sphereConstraints.end();
    });
    constraints.erase(it, constraints.end());
    
    // 释放球体碰撞约束的内存
    for (auto constraint : sphereConstraints) {
        delete constraint;
    }
    sphereConstraints.clear();
}

// 创建布料的粒子
void Cloth::createParticles(const dx::XMFLOAT3& position, float size, float mass) {
    particles.reserve(width * height);
    
    float stepX = size / (width - 1);
    float stepZ = size / (height - 1);
    
    // 将位置转换为XMVECTOR进行计算
    dx::XMVECTOR posVector = dx::XMLoadFloat3(&position);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 计算粒子位置
            dx::XMVECTOR offset = dx::XMVectorSet(x * stepX, 0.0f, y * stepZ, 0.0f);
            dx::XMVECTOR pos = dx::XMVectorAdd(posVector, offset);
            dx::XMFLOAT3 posFloat3;
            dx::XMStoreFloat3(&posFloat3, pos);
            
            // 左上角和右上角的粒子设为静态
            bool isStatic = (x == 0 && y == 0) || (x == width - 1 && y == 0);
            
            // 创建粒子
            particles.emplace_back(posFloat3, mass, isStatic);
        }
    }
}

// 创建布料的约束
void Cloth::createConstraints(float size) {
    distanceConstraints.clear();
    constraints.clear();
    
    float restLength = size / (width - 1);
    float compliance = 1e-7f; // 减小柔度值以增加布料刚性
    
    // 创建结构约束（相邻粒子之间）
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 水平约束
            if (x < width - 1) {
                int idx1 = y * width + x;
                int idx2 = y * width + (x + 1);
                distanceConstraints.emplace_back(&particles[idx1], &particles[idx2], restLength, compliance);
            }
            
            // 垂直约束
            if (y < height - 1) {
                int idx1 = y * width + x;
                int idx2 = (y + 1) * width + x;
                distanceConstraints.emplace_back(&particles[idx1], &particles[idx2], restLength, compliance);
            }
            
            // 对角约束（可选，增加稳定性）
            if (x < width - 1 && y < height - 1) {
                int idx1 = y * width + x;
                int idx2 = (y + 1) * width + (x + 1);
                distanceConstraints.emplace_back(&particles[idx1], &particles[idx2], restLength * std::sqrt(2.0f), compliance);
            }
            
            if (x < width - 1 && y > 0) {
                int idx1 = y * width + x;
                int idx2 = (y - 1) * width + (x + 1);
                distanceConstraints.emplace_back(&particles[idx1], &particles[idx2], restLength * std::sqrt(2.0f), compliance);
            }
        }
    }
    
    // 填充约束指针数组
    for (auto& constraint : distanceConstraints) {
        constraints.push_back(&constraint);
    }
}

// 计算布料的法线数据
void Cloth::computeNormals() {
    // 清空位置和法线向量（索引只计算一次）
    this->positions.clear();
    this->normals.clear();
    
    // 只有在索引为空时才计算索引数据（确保只计算一次）
    if (this->indices.empty()) {
        // 生成三角形面的索引数据
        for (int y = 0; y < height - 1; ++y) {
            for (int x = 0; x < width - 1; ++x) {
                // 第一个三角形：(x,y), (x+1,y), (x+1,y+1)
                int i1 = y * width + x;
                int i2 = y * width + x + 1;
                int i3 = (y + 1) * width + x + 1;
                
                this->indices.push_back(i1);
                this->indices.push_back(i2);
                this->indices.push_back(i3);
                
                // 第二个三角形：(x,y), (x+1,y+1), (x,y+1)
                i1 = y * width + x;
                i2 = (y + 1) * width + x + 1;
                i3 = (y + 1) * width + x;
                
                this->indices.push_back(i1);
                this->indices.push_back(i2);
                this->indices.push_back(i3);
            }
        }
    }
    
    // 计算法线（使用面法线的平均值）
    std::vector<dx::XMFLOAT3> vertexNormals(particles.size(), dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
    
    // 计算顶点法线
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            // 第一个三角形：(x,y), (x+1,y), (x+1,y+1)
            int i1 = y * width + x;
            int i2 = y * width + x + 1;
            int i3 = (y + 1) * width + x + 1;
            
            // 计算面法线 - 反转法线方向
            dx::XMVECTOR v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i2].position), dx::XMLoadFloat3(&particles[i1].position));
            dx::XMVECTOR v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i3].position), dx::XMLoadFloat3(&particles[i1].position));
            dx::XMVECTOR crossProduct = dx::XMVector3Cross(v2, v1); // 反转叉乘顺序以反转法线方向
            
            // 检查叉乘结果是否为零向量，避免NaN
            float lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(crossProduct));
            dx::XMVECTOR normal;
            
            if (lengthSquared > 0.0001f) {
                normal = dx::XMVector3Normalize(crossProduct);
            } else {
                // 如果叉乘结果接近零，使用默认法线
                normal = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 向上的法线
            }
            
            // 累加到顶点法线
            dx::XMVECTOR n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
            dx::XMVECTOR n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
            dx::XMVECTOR n3 = dx::XMLoadFloat3(&vertexNormals[i3]);
            
            dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
            dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
            dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
            
            // 第二个三角形：(x,y), (x+1,y+1), (x,y+1)
            i1 = y * width + x;
            i2 = (y + 1) * width + x + 1;
            i3 = (y + 1) * width + x;
            
            // 计算面法线 - 反转法线方向
            v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i2].position), dx::XMLoadFloat3(&particles[i1].position));
            v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i3].position), dx::XMLoadFloat3(&particles[i1].position));
            crossProduct = dx::XMVector3Cross(v2, v1); // 反转叉乘顺序以反转法线方向
            
            // 检查叉乘结果是否为零向量，避免NaN
            lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(crossProduct));
            
            if (lengthSquared > 0.0001f) {
                normal = dx::XMVector3Normalize(crossProduct);
            } else {
                // 如果叉乘结果接近零，使用默认法线
                normal = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 向上的法线
            }
            
            // 累加到顶点法线
            n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
            n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
            n3 = dx::XMLoadFloat3(&vertexNormals[i3]);
            
            dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
            dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
            dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
        }
    }
    
    // 归一化顶点法线并准备位置和法线数据
    for (size_t i = 0; i < particles.size(); ++i) {
        this->positions.push_back(particles[i].position);
        
        // 归一化顶点法线，添加检查避免对零向量进行归一化
        dx::XMVECTOR n = dx::XMLoadFloat3(&vertexNormals[i]);
        float lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(n));
        dx::XMFLOAT3 normalizedNormal;
        
        if (lengthSquared > 0.0001f) {
            n = dx::XMVector3Normalize(n);
            dx::XMStoreFloat3(&normalizedNormal, n);
        } else {
            // 如果顶点法线接近零，使用默认法线
            normalizedNormal = dx::XMFLOAT3(0.0f, 1.0f, 0.0f); // 向上的法线
        }
        
        this->normals.push_back(normalizedNormal);
    }
    

}