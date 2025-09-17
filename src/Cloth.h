#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include <DirectXMath.h>
#include "Particle.h"
#include "Constraint.h"
#include "DistanceConstraint.h"
#include "XPBDSolver.h"
#include "Primitive.h"
#include <cstdint> // For uint32_t

// 前向声明
class SphereCollisionConstraint;

// 为了方便使用，创建一个命名空间别名
namespace dx = DirectX;

class Cloth : public Primitive {
public:
    // 构造函数：创建一个布料对象
    // 参数：
    //   position - 布料左下角的初始位置
    //   width - 布料宽度方向的粒子数
    //   height - 布料高度方向的粒子数
    //   size - 布料的实际物理尺寸（以米为单位）
    //   mass - 每个粒子的质量
    Cloth(const dx::XMFLOAT3& position, int width, int height, float size, float mass);
    
    // 析构函数
    ~Cloth() override;
    
    // 更新布料状态
    void update(float deltaTime) override;
    
    // 获取布料的所有粒子
    const std::vector<Particle>& getParticles() const {
        return particles;
    }
    
    // 获取布料的宽度（粒子数）
    int getWidth() const {
        return width;
    }
    
    // 获取布料的高度（粒子数）
    int getHeight() const {
        return height;
    }
    
    // 使用传统方法检查布料粒子与球体的碰撞
    // 参数:
    //   sphereCenter - 球体中心位置
    //   sphereRadius - 球体半径
    void checkSphereCollision(const dx::XMFLOAT3& sphereCenter, float sphereRadius);
    
    // 添加基于XPBD约束的球体碰撞检测
    // 参数:
    //   sphereCenter - 球体中心位置
    //   sphereRadius - 球体半径
    void addSphereCollisionConstraint(const dx::XMFLOAT3& sphereCenter, float sphereRadius);
    
    // 清除所有球体碰撞约束
    void clearSphereCollisionConstraints();
    
    // 设置是否使用XPBD碰撞约束
    void setUseXPBDCollision(bool use) {
        if (useXPBDCollision != use) {
            useXPBDCollision = use;
            
            // 如果启用XPBD碰撞，将所有球体碰撞约束添加到总约束列表
            // 如果禁用XPBD碰撞，从总约束列表中删除所有球体碰撞约束
            // 注意：使用直接比较而非std::find，避免类型转换问题
            auto it = constraints.begin();
            while (it != constraints.end()) {
                bool isSphereConstraint = false;
                for (auto sphereConstraint : sphereConstraints) {
                    if (*it == reinterpret_cast<Constraint*>(sphereConstraint)) {
                        isSphereConstraint = true;
                        break;
                    }
                }
                if (isSphereConstraint) {
                    it = constraints.erase(it);
                } else {
                    ++it;
                }
            }
            
            if (useXPBDCollision) {
                for (auto constraint : sphereConstraints) {
                    constraints.push_back(reinterpret_cast<Constraint*>(constraint));
                }
            }
        }
    }
    
    // 获取是否使用XPBD碰撞约束
    bool getUseXPBDCollision() const { return useXPBDCollision; }

    // 获取布料的顶点位置数据
    const std::vector<dx::XMFLOAT3>& getPositions() const override { return positions; }
    
    // 获取布料的顶点法线数据
    const std::vector<dx::XMFLOAT3>& getNormals() const override { return normals; }
    
    // 获取布料的索引数据
    const std::vector<uint32_t>& getIndices() const override { return indices; }

private:
    // 创建布料的粒子
    void createParticles(const dx::XMFLOAT3& position, float size, float mass);
    
    // 创建布料的约束
    void createConstraints(float size);
    
    // 初始化球体碰撞约束（一次性创建，避免每次重建）
    void initializeSphereCollisionConstraints(const dx::XMFLOAT3& sphereCenter, float sphereRadius);
    
    // 布料的尺寸参数
    int width; // 宽度方向的粒子数
    int height; // 高度方向的粒子数
    
    // 粒子和约束
    std::vector<Particle> particles; // 布料的所有粒子
    std::vector<DistanceConstraint> distanceConstraints; // 布料的所有距离约束
    std::vector<Constraint*> constraints; // 约束指针数组，用于传递给求解器
    std::vector<SphereCollisionConstraint*> sphereConstraints; // 球体碰撞约束
    bool useXPBDCollision; // 是否使用XPBD碰撞约束
    
    // XPBD求解器
    XPBDSolver solver; // 用于求解布料的物理行为
    
    // 重力
    dx::XMFLOAT3 gravity; // 作用在布料上的重力
    
    // 渲染数据
    std::vector<dx::XMFLOAT3> positions; // 布料顶点位置数据
    std::vector<dx::XMFLOAT3> normals; // 布料顶点法线数据
    std::vector<uint32_t> indices; // 布料索引数据
};

#endif // CLOTH_H
