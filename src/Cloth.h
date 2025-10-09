#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include <DirectXMath.h>
#include "Particle.h"
#include "Constraint.h"
#include "DistanceConstraint.h"
#include "LRAConstraint.h"
#include "DihedralBendingConstraint.h"
#include "XPBDSolver.h"
#include "Mesh.h"
#include "RALResource.h"
#include "IRALDevice.h"
#include <cstdint> // For uint32_t


// 为了方便使用，创建一个命名空间别名
namespace dx = DirectX;

enum ClothParticleMassMode
{
	ClothParticleMassMode_FixedTotalMass, // 固定总质量，粒子质量随分辨率变化
	ClothParticleMassMode_FixedParticleMass, // 固定粒子质量，总质量随分辨率变化
};

class Cloth : public Mesh 
{
public:
    // 构造函数：创建一个布料对象
    // 参数：
    //   widthResolution - 布料宽度方向的粒子数
    //   heightResolution - 布料高度方向的粒子数
    //   size - 布料的实际物理尺寸（以米为单位）
    //   mass - 每个粒子的质量
	//   massMode - 质量模式
    Cloth(int widthResolution, int heightResolution, float size, float mass, ClothParticleMassMode massMode);
    
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

    // 获取距离约束的柔度
    float GetDistanceConstraintCompliance() const
    {
        return m_distanceConstraintCompliance;
    }

    // 设置距离约束的柔度
    void SetDistanceConstraintCompliance(float compliance)
    {
        m_distanceConstraintCompliance = compliance;
    }

    // 获取距离约束的阻尼
    float GetDistanceConstraintDamping() const
    {
        return m_distanceConstraintDamping;
    }

    // 设置距离约束的阻尼
    void SetDistanceConstraintDamping(float damping)
    {
        m_distanceConstraintDamping = damping;
    }

    // 获取是否增加对角约束
    bool GetAddDiagonalConstraints() const
    {
        return m_addDiagonalConstraints;
    }

    // 设置是否增加对角约束
    void SetAddDiagonalConstraints(bool add)
    {
        m_addDiagonalConstraints = add;
    }

    // 设置是否增加Bending约束
    void SetAddBendingConstraints(bool add)
    {
        m_addBendingConstraints = add;
    }

    // 获取是否增加Bending约束
    bool GetAddBendingConstraints() const
    {
        return m_addBendingConstraints;
    }

    // 获取弯曲约束的柔度
    float GetBendingConstraintCompliance() const
    {
        return m_bendingConstraintCompliance;
    }

    // 设置弯曲约束的柔度
    void SetBendingConstraintCompliance(float compliance)
    {
        m_bendingConstraintCompliance = compliance;
    }

    // 获取弯曲约束的阻尼
    float GetBendingConstraintDamping() const
    {
        return m_bendingConstraintDamping;
    }

    // 设置弯曲约束的阻尼
    void SetBendingConstraintDamping(float damping)
    {
        m_bendingConstraintDamping = damping;
    }

    // 获取是否增加LRA约束
    float GetAddLRAConstraints() const
    {
        return m_addLRAConstraints;
    }

    // 设置是否增加LRA约束
    void SetAddLRAConstraints(bool add)
    {
        m_addLRAConstraints = add;
    }

    // 获取LRA约束最大拉伸量
    float GetLRAMaxStretch() const
    { 
        return m_LRAMaxStrech; 
    }

    // 设置LRA约束最大拉伸量
    void SetLRAMaxStretch(float maxStretch)
    {
        m_LRAMaxStrech = maxStretch;
    }

    // 获取LRA约束的柔度
    float GetLRAConstraintCompliance() const
    {
        return m_LRAConstraintCompliance;
    }

    // 设置LRA约束的柔度
    void SetLRAConstraintCompliance(float compliance)
    {
        m_LRAConstraintCompliance = compliance;
    }

    // 获取是否增加二面角约束
    bool GetAddDihedralBendingConstraints() const
    {
        return m_addDihedralBendingConstraints;
    }

    // 设置是否增加二面角约束
    void SetAddDihedralBendingConstraints(bool add)
    {
        m_addDihedralBendingConstraints = add;
    }

    // 获取二面角约束的柔度
    float GetDihedralBendingConstraintCompliance() const
    {
        return m_dihedralBendingConstraintCompliance;
    }

    // 设置二面角约束的柔度
    void SetDihedralBendingConstraintCompliance(float compliance)
    {
        m_dihedralBendingConstraintCompliance = compliance;
    }

    // 获取二面角约束的阻尼
    float GetDihedralBendingConstraintDamping() const
    {
        return m_dihedralBendingConstraintDamping;
    }

    // 设置二面角约束的阻尼
    void SetDihedralBendingConstraintClamping(float c)
    {
        m_dihedralBendingConstraintDamping = c;
    }
    
    // 获取每个粒子的质量
    float GetMass() const 
    { 
        return m_mass;
    }

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

    // 增加距离约束
    void AddDistanceConstraint(const DistanceConstraint& constraint);

    // 增加LRA约束
    void AddLRAConstraint(const LRAConstraint& constraint);

    // 增加二面角约束
    void AddDihedralBendingConstraint(const DihedralBendingConstraint& constraint);

private:
    // 布料的尺寸参数
    int m_widthResolution; // 宽度方向的粒子数
    int m_heightResolution; // 高度方向的粒子数
    float m_size;
    float m_mass;
	ClothParticleMassMode m_massMode; // 质量模式
    
    // 粒子和约束
    std::vector<Particle> m_particles; // 布料的所有粒子
    std::vector<DistanceConstraint> m_distanceConstraints; // 布料的所有距离约束
    std::vector<Constraint*> m_CollisionConstraints; // 碰撞约束
    std::vector<LRAConstraint> m_lraConstraints; // LRA约束
    std::vector<DihedralBendingConstraint> m_dihedralBendingConstraints; // 二面角约束
    float m_distanceConstraintCompliance; // 距离约束的柔度系数
    float m_distanceConstraintDamping;  // 距离约束的阻尼系数

	bool m_addDiagonalConstraints; // 是否增加对角线约束
    bool m_addBendingConstraints; // 是否增加弯曲约束
    float m_bendingConstraintCompliance; // 弯曲约束的柔度系数
    float m_bendingConstraintDamping;  // 弯曲约束的阻尼系数

    bool m_addLRAConstraints;    // 是否增加LRA约束
    float m_LRAConstraintCompliance; // LRA约束的柔度系数
    float m_LRAConstraintDamping; // LRA约束的阻尼系数
    float m_LRAMaxStrech;       // LRA最大拉伸量

    bool m_addDihedralBendingConstraints; // 是否增加二面角bending约束
    float m_dihedralBendingConstraintCompliance; // 二面角约束的柔度系数
    float m_dihedralBendingConstraintDamping; // 二面角约束的阻尼系数

    float m_sphereCollisionConstraintCompliance; // 球面碰撞约束的柔度系数
    float m_sphereCollisionConstraintDamping; // 球面碰撞约束的阻尼系数

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

    friend class XPBDSolver;
};

#endif // CLOTH_H
