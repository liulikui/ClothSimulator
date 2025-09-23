#ifndef SPHERE_H
#define SPHERE_H

#include "Mesh.h"
#include "RALResource.h"
#include "DX12RALDevice.h"
#include <DirectXMath.h>
#include <vector>
#include <cstdint>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

class Sphere : public Mesh {
public:
    // 构造函数：创建一个球体对象
    // 参数：
    //   radius - 球体半径
    //   sectors - 球体的经度分段数
    //   stacks - 球体的纬度分段数
    Sphere(float radius = 1.0f,
           uint32_t sectors = 32,
           uint32_t stacks = 32);

    // 析构函数
    ~Sphere() override = default;

    // 更新球体状态
    void Update(IRALGraphicsCommandList* commandList, float deltaTime) override;

    // 初始化球体
    bool Initialize(IRALDevice* device);

    // 初始化Mesh
    virtual void OnSetupMesh(IRALDevice* device, PrimitiveMesh& mesh) override;

    // 获取球体的顶点位置数据
    const std::vector<dx::XMFLOAT3>& GetPositions() const override
    {
        return m_positions;
    }

    // 获取球体的顶点法线数据
    const std::vector<dx::XMFLOAT3>& GetNormals() const override
    {
        return m_normals;
    }

    // 获取球体的索引数据
    const std::vector<uint32_t>& GetIndices() const override
    {
        return m_indices;
    }

    // 获取球体半径
    float GetRadius() const
    {
        return m_radius;
    }

    // 设置球体半径
    void SetRadius(float newRadius);

    // 获取球体中心位置
    const dx::XMFLOAT3& GetCenter() const
    {
        return m_center;
    }

    // 设置球体中心位置
    void SetCenter(const dx::XMFLOAT3& newCenter);

    // 获取经度分段数
    uint32_t GetSectors() const
    {
        return m_sectors;
    }

    // 获取纬度分段数
    uint32_t GetStacks() const
    {
        return m_stacks;
    }

private:
    // 球体参数
    dx::XMFLOAT3 m_center; // 球体中心位置
    float m_radius;        // 球体半径
    uint32_t m_sectors;    // 经度分段数
    uint32_t m_stacks;     // 纬度分段数

    // 渲染数据
    std::vector<dx::XMFLOAT3> m_positions; // 顶点位置数据
    std::vector<dx::XMFLOAT3> m_normals;   // 顶点法线数据
    std::vector<uint32_t> m_indices;       // 索引数据

    // 生成球体的顶点和索引数据
    void GenerateSphereData();
};

#endif // SPHERE_H