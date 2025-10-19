#include "Sphere.h"
#include <DirectXMath.h>
#include <cmath>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

Sphere::Sphere(float radius, uint32_t sectors, uint32_t stacks)
    : m_radius(radius), m_sectors(sectors), m_stacks(stacks), m_center(dx::XMFLOAT3(0.0f, 0.0f, 0.0f))
{
    // 生成球体数据
    GenerateSphereData();
}

void Sphere::Update(IRALGraphicsCommandList* commandList, float deltaTime)
{
    // 球体默认不需要复杂的更新逻辑
    // 可以在这里添加动画或其他动态行为
}

bool Sphere::Initialize(IRALDevice* device)
{
    if (!device || m_positions.empty() || m_indices.empty())
    {
        return false;
    }

    return true;
}

void Sphere::OnSetupMesh(IRALDevice* device, PrimitiveMesh& mesh)
{
    // 创建顶点数据（位置 + 法线）
    std::vector<uint8_t> vertexData(m_positions.size() * sizeof(dx::XMFLOAT3) * 2);
    for (size_t i = 0; i < m_positions.size(); ++i)
    {
        size_t positionOffset = i * sizeof(dx::XMFLOAT3) * 2;
        size_t normalOffset = positionOffset + sizeof(dx::XMFLOAT3);

        memcpy(&vertexData[positionOffset], &m_positions[i], sizeof(dx::XMFLOAT3));
        memcpy(&vertexData[normalOffset], &m_normals[i], sizeof(dx::XMFLOAT3));
    }

    // 创建顶点缓冲区
    mesh.vertexBuffer = device->CreateVertexBuffer(
        vertexData.size(),
        6 * sizeof(float),// 顶点 stride（3个位置分量 + 3个法线分量）
        true,
        vertexData.data(),
		L"SphereVB"
    );

    // 创建索引缓冲区
    mesh.indexBuffer = device->CreateIndexBuffer(
        m_indices.size(),
        true,// 32位索引
        true,
        m_indices.data(),
        L"SphereIB"
    );
}

void Sphere::SetRadius(float newRadius)
{
    if (m_radius != newRadius)
    {
        m_radius = newRadius;
        GenerateSphereData();
    }
}

void Sphere::SetCenter(const dx::XMFLOAT3& newCenter)
{
    if (m_center.x != newCenter.x || m_center.y != newCenter.y || m_center.z != newCenter.z)
    {
        m_center = newCenter;
        SetPosition(m_center);
    }
}

void Sphere::GenerateSphereData()
{
    // 清空现有数据
    m_positions.clear();
    m_normals.clear();
    m_indices.clear();

    // 生成顶点和法线
    for (uint32_t i = 0; i <= m_stacks; ++i)
    {
        float phi = dx::XMConvertToRadians(180.0f * static_cast<float>(i) / m_stacks); // 极角
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (uint32_t j = 0; j <= m_sectors; ++j)
        {
            float theta = dx::XMConvertToRadians(360.0f * static_cast<float>(j) / m_sectors); // 方位角
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // 添加顶点位置
            dx::XMFLOAT3 pos;
            pos.x = m_radius * sinPhi * cosTheta;
            pos.y = m_radius * cosPhi;
            pos.z = m_radius * sinPhi * sinTheta;
            m_positions.push_back(pos);

            // 添加法线（单位向量）
            dx::XMFLOAT3 normal;
            normal.x = sinPhi * cosTheta;
            normal.y = cosPhi;
            normal.z = sinPhi * sinTheta;
            m_normals.push_back(normal);
        }
    }

    // 生成索引
    for (uint32_t i = 0; i < m_stacks; ++i)
    {
        uint32_t row1 = i * (m_sectors + 1);
        uint32_t row2 = (i + 1) * (m_sectors + 1);

        for (uint32_t j = 0; j < m_sectors; ++j)
        {
            // 第一个三角形
            m_indices.push_back(row1 + j);
            m_indices.push_back(row2 + j + 1);
            m_indices.push_back(row1 + j + 1);

            // 第二个三角形
            m_indices.push_back(row1 + j);
            m_indices.push_back(row2 + j);
            m_indices.push_back(row2 + j + 1);
        }
    }
}