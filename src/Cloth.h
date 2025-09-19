#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include <DirectXMath.h>
#include "Particle.h"
#include "Constraint.h"
#include "DistanceConstraint.h"
#include "XPBDSolver.h"
#include "Mesh.h"
#include "RALResource.h"
#include "DX12Renderer.h"
#include <cstdint> // For uint32_t

// 前向声明
class SphereCollisionConstraint;

// 为了方便使用，创建一个命名空间别名
namespace dx = DirectX;

class Cloth : public Mesh 
{
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
    void Update(IRALGraphicsCommandList* commandList, float deltaTime) override;
    
    // 初始化布料
    bool Initialize(DX12Renderer* renderer);
    
    // 获取布料的所有粒子
    const std::vector<Particle>& GetParticles() const
    {
        return m_particles;
    }
    
    // 获取布料的宽度（粒子数）
    int GetWidth() const
    {
        return m_width;
    }
    
    // 获取布料的高度（粒子数）
    int GetHeight() const
    {
        return m_height;
    }
    
    // 使用传统方法检查布料粒子与球体的碰撞
    // 参数:
    //   sphereCenter - 球体中心位置
    //   sphereRadius - 球体半径
    void CheckSphereCollision(const dx::XMFLOAT3& sphereCenter, float sphereRadius);
    
    // 添加基于XPBD约束的球体碰撞检测
    // 参数:
    //   sphereCenter - 球体中心位置
    //   sphereRadius - 球体半径
    void AddSphereCollisionConstraint(const dx::XMFLOAT3& sphereCenter, float sphereRadius);
    
    // 清除所有球体碰撞约束
    void ClearSphereCollisionConstraints();
    
    // 设置是否使用XPBD碰撞约束
    void SetUseXPBDCollision(bool use)
    {
        if (m_useXPBDCollision != use)
        {
            m_useXPBDCollision = use;
            
            // 如果启用XPBD碰撞，将所有球体碰撞约束添加到总约束列表
            // 如果禁用XPBD碰撞，从总约束列表中删除所有球体碰撞约束
            // 注意：使用直接比较而非std::find，避免类型转换问题
            auto it = m_constraints.begin();
            while (it != m_constraints.end())
            {
                bool isSphereConstraint = false;
                for (auto sphereConstraint : m_sphereConstraints)
                {
                    if (*it == reinterpret_cast<Constraint*>(sphereConstraint))
                    {
                        isSphereConstraint = true;
                        break;
                    }
                }

                if (isSphereConstraint)
                {
                    it = m_constraints.erase(it);
                } 
                else 
                {
                    ++it;
                }
            }
            
            if (m_useXPBDCollision)
            {
                for (auto constraint : m_sphereConstraints)
                {
                    m_constraints.push_back(reinterpret_cast<Constraint*>(constraint));
                }
            }
        }
    }
    
    // 获取是否使用XPBD碰撞约束
    bool GetUseXPBDCollision() const { return m_useXPBDCollision; }

    // 获取布料的顶点位置数据
    const std::vector<dx::XMFLOAT3>& GetPositions() const override { return m_positions; }
    
    // 获取布料的顶点法线数据
    const std::vector<dx::XMFLOAT3>& GetNormals() const override { return m_normals; }
    
    // 获取布料的索引数据
    const std::vector<uint32_t>& GetIndices() const override { return m_indices; }

private:
    // 创建布料的粒子
    void CreateParticles(const dx::XMFLOAT3& position, float size, float mass);
    
    // 创建布料的约束
    void CreateConstraints(float size);
    
    // 初始化球体碰撞约束（一次性创建，避免每次重建）
    void InitializeSphereCollisionConstraints(const dx::XMFLOAT3& sphereCenter, float sphereRadius);
    
    // 布料的尺寸参数
    int m_width; // 宽度方向的粒子数
    int m_height; // 高度方向的粒子数
    
    // 粒子和约束
    std::vector<Particle> m_particles; // 布料的所有粒子
    std::vector<DistanceConstraint> m_distanceConstraints; // 布料的所有距离约束
    std::vector<Constraint*> m_constraints; // 约束指针数组，用于传递给求解器
    std::vector<SphereCollisionConstraint*> m_sphereConstraints; // 球体碰撞约束
    bool m_useXPBDCollision; // 是否使用XPBD碰撞约束
    
    // XPBD求解器
    XPBDSolver m_solver; // 用于求解布料的物理行为
    
    // 重力
    dx::XMFLOAT3 m_gravity; // 作用在布料上的重力
    
    // 渲染数据
    std::vector<dx::XMFLOAT3> m_positions; // 布料顶点位置数据
    std::vector<dx::XMFLOAT3> m_normals; // 布料顶点法线数据
    std::vector<uint32_t> m_indices; // 布料索引数据
    
    // 渲染资源
    TSharePtr<IRALVertexBuffer> m_vertexBuffer;
    TSharePtr<IRALIndexBuffer> m_indexBuffer;
};

#endif // CLOTH_H
