#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include <DirectXMath.h>
#include "Particle.h"
#include "Constraint.h"
#include "DistanceConstraint.h"
#include "LRAConstraint.h"
#include "XPBDSolver.h"
#include "Mesh.h"
#include "RALResource.h"
#include "DX12RALDevice.h"
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
    //   widthResolution - 布料宽度方向的粒子数
    //   heightResolution - 布料高度方向的粒子数
    //   size - 布料的实际物理尺寸（以米为单位）
    //   mass - 每个粒子的质量
    Cloth(int widthResolution, int heightResolution, float size, float mass);
    
    // 析构函数
    ~Cloth() override;
    
    // 更新布料状态
    void Update(IRALGraphicsCommandList* commandList, float deltaTime) override;
    
    // 初始化布料
    bool Initialize(IRALDevice* device);
    
    // 初始化Mesh
    virtual void OnSetupMesh(IRALDevice* device, PrimitiveMesh& mesh) override;

    // Update Mesh
    virtual void OnUpdateMesh(IRALDevice* device, PrimitiveMesh& mesh) override;

    // 获取布料的所有粒子
    const std::vector<Particle>& GetParticles() const
    {
        return m_particles;
    }
    
    // 获取布料的宽度（粒子数）
    int GetWidthResolution() const
    {
        return m_widthResolution;
    }
    
    // 获取布料的高度（粒子数）
    int GetHeightResolution() const
    {
        return m_heightResolution;
    }
    
    // 获取迭代次数
    uint32_t GetIteratorCount() const
    {
        return m_iteratorCount;
    }

    // 设置迭代次数
    void SetIteratorCount(uint32_t count)
    {
        m_iteratorCount = count;
    }

    // 获取子迭代次数
    uint32_t GetSubIteratorCount() const
    {
        return m_subIteratorCount;
    }

    // 设置子迭代次数
    void SetSubIteratorCount(uint32_t count)
    {
        m_subIteratorCount = count;
    }

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

    // 设置是否增加LRA约束
    void SetAddLRAConstraint(bool add)
    {
        m_addLRAConstraint = add;
    }

    // 获取是否增加LRA约束
    bool GetAddLRAConstraint() const { return m_addLRAConstraint; }

    // 设置LRA约束最大拉伸量
    void SetLRAMaxStretch(float maxStretch)
    {
        m_LRAMaxStrech = maxStretch;
    }

    // 获取LRA约束最大拉伸量
    float GetLRAMaxStretch() const { return m_LRAMaxStrech; }
    
    // 获取每个粒子的质量
    float GetMass() const { return m_mass; }

    // 获取布料的顶点位置数据
    const std::vector<dx::XMFLOAT3>& GetPositions() const override { return m_positions; }
    
    // 获取布料的顶点法线数据
    const std::vector<dx::XMFLOAT3>& GetNormals() const override { return m_normals; }
    
    // 获取布料的索引数据
    const std::vector<uint32_t>& GetIndices() const override { return m_indices; }

    // 初始化球体碰撞约束（一次性创建，避免每次重建）
    void InitializeSphereCollisionConstraints(const dx::XMFLOAT3& sphereCenter, float sphereRadius);

private:
    // 创建布料的粒子
    void CreateParticles();
    
    // 创建布料的约束
    void CreateConstraints();

    // 计算法线
    void ComputeNormals();

private:
    // 布料的尺寸参数
    int m_widthResolution; // 宽度方向的粒子数
    int m_heightResolution; // 高度方向的粒子数
    float m_size;
    float m_mass;
    
    // 粒子和约束
    std::vector<Particle> m_particles; // 布料的所有粒子
    std::vector<DistanceConstraint> m_distanceConstraints; // 布料的所有距离约束
    std::vector<Constraint*> m_constraints; // 约束指针数组，用于传递给求解器
    std::vector<SphereCollisionConstraint*> m_sphereConstraints; // 球体碰撞约束
    std::vector<LRAConstraint> m_lraConstraints; // LRA约束
    bool m_useXPBDCollision; // 是否使用XPBD碰撞约束
    bool m_addLRAConstraint; // 是否增加LRA约束
    float m_LRAMaxStrech;// LRA最大拉伸量
    
    // XPBD求解器
    XPBDSolver m_solver; // 用于求解布料的物理行为
    
    // 重力
    dx::XMFLOAT3 m_gravity; // 作用在布料上的重力
    
    // 渲染数据
    std::vector<dx::XMFLOAT3> m_positions; // 布料顶点位置数据
    std::vector<dx::XMFLOAT3> m_normals; // 布料顶点法线数据
    std::vector<uint32_t> m_indices; // 布料索引数据

    uint32_t m_iteratorCount;   // 迭代次数
    uint32_t m_subIteratorCount;   // 子迭代次数
};

#endif // CLOTH_H
