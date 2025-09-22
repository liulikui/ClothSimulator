#include "Cloth.h"
#include <iostream>
#include <DirectXMath.h>
#include <algorithm>
#include "SphereCollisionConstraint.h"
#include "DX12RALResource.h"

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 声明外部函数
extern int& GetCollisionConstraintCount();

// 构造函数：创建一个布料对象
// 参数：
//   width - 布料宽度方向的粒子数
//   height - 布料高度方向的粒子数
//   size - 布料的实际物理尺寸（以米为单位）
//   mass - 每个粒子的质量
Cloth::Cloth(int width, int height, float size, float mass)
    : Mesh(), m_width(width), m_height(height), m_solver(m_particles, m_constraints), m_useXPBDCollision(true)
{
    // 创建粒子
    CreateParticles(size, mass);
    
    // 创建布料的约束
    CreateConstraints(size);
    
    // 设置重力为标准地球重力
    m_gravity = dx::XMFLOAT3(0.0f, -9.8f, 0.0f);
    m_solver.SetGravity(m_gravity);
    
    // 设置求解器参数（增加迭代次数以提高约束求解质量）
    m_solver.SetIterations(80); // 显著增加迭代次数以提高高分辨率布料和碰撞的稳定性
}

// 析构函数
Cloth::~Cloth()
{
    // 清除球体碰撞约束
    ClearSphereCollisionConstraints();
    
    // 清除距离约束
    for (auto& constraint : m_distanceConstraints) {
        // 注意：distanceConstraints中的对象是直接存储的，不需要delete
    }
    
    // 清除约束指针数组中的其他约束（如果有）
    m_constraints.clear();
}

// 初始化布料的顶点和索引缓冲区
bool Cloth::Initialize(DX12Renderer* renderer)
{
    // 确保renderer不为空
    if (!renderer)
    {
        std::cerr << "Cloth::Initialize: renderer is null" << std::endl;
        return false;
    }

    // 生成布料的顶点和索引数据
    // 首先生成索引数据（如果尚未生成）
    if (m_indices.empty())
    {
        // 生成三角形面的索引数据
        for (int y = 0; y < m_height - 1; ++y) {
            for (int x = 0; x < m_width - 1; ++x) {
                // 第一个三角形：(x,y), (x+1,y), (x+1,y+1)
                int i1 = y * m_width + x;
                int i2 = y * m_width + x + 1;
                int i3 = (y + 1) * m_width + x + 1;

                m_indices.push_back(i1);
                m_indices.push_back(i2);
                m_indices.push_back(i3);

                // 第二个三角形：(x,y), (x+1,y+1), (x,y+1)
                i1 = y * m_width + x;
                i2 = (y + 1) * m_width + x + 1;
                i3 = (y + 1) * m_width + x;

                m_indices.push_back(i1);
                m_indices.push_back(i2);
                m_indices.push_back(i3);
            }
        }
    }
   
    return true;
}

void Cloth::OnSetupMesh(DX12Renderer* renderer, PrimitiveMesh& mesh)
{
    // 生成顶点数据（位置和法线）
    std::vector<float> vertexData;

    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        // 添加顶点位置
        vertexData.push_back(m_particles[i].position.x);
        vertexData.push_back(m_particles[i].position.y);
        vertexData.push_back(m_particles[i].position.z);

        // 添加顶点法线
        vertexData.push_back(m_normals[i].x);
        vertexData.push_back(m_normals[i].y);
        vertexData.push_back(m_normals[i].z);
    }

    // 创建顶点缓冲区
    size_t vertexBufferSize = vertexData.size() * sizeof(float);
    mesh.vertexBuffer = renderer->CreateVertexBuffer(
        vertexBufferSize,
        6 * sizeof(float),// 顶点 stride（3个位置分量 + 3个法线分量）
        true
    );

    // 创建索引缓冲区
    size_t indexBufferSize = m_indices.size() * sizeof(uint32_t);
    mesh.indexBuffer = renderer->CreateIndexBuffer(
        m_indices.size(),
        true, // 32位索引
        true
    );

    // 上传顶点数据
    renderer->UploadBuffer(mesh.vertexBuffer.Get(), (const char*)vertexData.data(), vertexBufferSize);

    // 上传索引数据
    renderer->UploadBuffer(mesh.indexBuffer.Get(), (const char*)m_indices.data(), indexBufferSize);
}

void Cloth::OnUpdateMesh(DX12Renderer* renderer, PrimitiveMesh& mesh)
{
    // 生成顶点数据（位置和法线）
    std::vector<float> vertexData;

    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        // 添加顶点位置
        vertexData.push_back(m_particles[i].position.x);
        vertexData.push_back(m_particles[i].position.y);
        vertexData.push_back(m_particles[i].position.z);

        // 添加顶点法线
        vertexData.push_back(m_normals[i].x);
        vertexData.push_back(m_normals[i].y);
        vertexData.push_back(m_normals[i].z);
    }

    size_t vertexBufferSize = vertexData.size() * sizeof(float);

    // 上传顶点数据
    renderer->UploadBuffer(mesh.vertexBuffer.Get(), (const char*)vertexData.data(), vertexBufferSize);
}

// 初始化球体碰撞约束（一次性创建，避免每帧重建）
void Cloth::InitializeSphereCollisionConstraints(const dx::XMFLOAT3& sphereCenter, float sphereRadius)
{
    dx::XMFLOAT3 relativeCenter;
    dx::XMStoreFloat3(&relativeCenter, dx::XMVectorSubtract(dx::XMLoadFloat3(&sphereCenter), dx::XMLoadFloat3(&GetPosition())));

    // 为每个非静态粒子创建一个球体碰撞约束，并设置更小的柔度值以提高碰撞刚性
    for (auto& particle : m_particles)
    {
        if (!particle.isStatic)
        {
            SphereCollisionConstraint* constraint = new SphereCollisionConstraint(
                &particle, relativeCenter, sphereRadius, 1e-9f); // 减小柔度值以增加刚性
            m_sphereConstraints.push_back(constraint);
        }
    }
    
    for (auto constraint : m_sphereConstraints)
    {
        m_constraints.push_back(reinterpret_cast<Constraint*>(constraint));
    }
}

void Cloth::ComputeNormals()
{
    m_normals.clear();

    // 计算法线（使用面法线的平均值）
    std::vector<dx::XMFLOAT3> vertexNormals(m_particles.size(), dx::XMFLOAT3(0.0f, 0.0f, 0.0f));

    // 计算顶点法线
    for (int y = 0; y < m_height - 1; ++y) {
        for (int x = 0; x < m_width - 1; ++x) {
            // 第一个三角形：(x,y), (x+1,y), (x+1,y+1)
            int i1 = y * m_width + x;
            int i2 = y * m_width + x + 1;
            int i3 = (y + 1) * m_width + x + 1;

            // 计算面法线 - 反转法线方向
            dx::XMVECTOR v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), dx::XMLoadFloat3(&m_particles[i1].position));
            dx::XMVECTOR v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), dx::XMLoadFloat3(&m_particles[i1].position));
            dx::XMVECTOR crossProduct = dx::XMVector3Cross(v2, v1); // 反转叉乘顺序以反转法线方向

            // 检查叉乘结果是否为零向量，避免NaN
            float lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(crossProduct));
            dx::XMVECTOR normal;

            if (lengthSquared > 0.0001f) 
            {
                normal = dx::XMVector3Normalize(crossProduct);
            }
            else
            {
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
            i1 = y * m_width + x;
            i2 = (y + 1) * m_width + x + 1;
            i3 = (y + 1) * m_width + x;

            // 计算面法线 - 反转法线方向
            v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), dx::XMLoadFloat3(&m_particles[i1].position));
            v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), dx::XMLoadFloat3(&m_particles[i1].position));
            crossProduct = dx::XMVector3Cross(v2, v1); // 反转叉乘顺序以反转法线方向

            // 检查叉乘结果是否为零向量，避免NaN
            lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(crossProduct));

            if (lengthSquared > 0.0001f)
            {
                normal = dx::XMVector3Normalize(crossProduct);
            }
            else
            {
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
    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        // 归一化顶点法线，添加检查避免对零向量进行归一化
        dx::XMVECTOR n = dx::XMLoadFloat3(&vertexNormals[i]);
        float lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(n));
        dx::XMFLOAT3 normalizedNormal;

        if (lengthSquared > 0.0001f) {
            n = dx::XMVector3Normalize(n);
            dx::XMStoreFloat3(&normalizedNormal, n);
        }
        else
        {
            // 如果顶点法线接近零，使用默认法线
            normalizedNormal = dx::XMFLOAT3(0.0f, 1.0f, 0.0f); // 向上的法线
        }

        m_normals.push_back(normalizedNormal);
    }
}

// 更新布料状态
void Cloth::Update(IRALGraphicsCommandList* commandList, float deltaTime) {
    // 重置碰撞约束计数
    GetCollisionConstraintCount() = 0;
    
    // 添加重力
    m_solver.SetGravity(m_gravity);
    
    // 设置球体参数
    dx::XMFLOAT3 sphereCenter(0.0f, 0.0f, 0.0f); // 与Main.cpp中的注释一致，使布料中心对准球体(0,0,0)
    float sphereRadius = 2.0f;

    // 使用XPBD求解器更新布料状态
    m_solver.Step(deltaTime);
    
    // 计算布料的法线数据
    // 清空位置和法线向量（索引只计算一次）
    m_positions.clear();
    m_normals.clear();
    
    ComputeNormals();

    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        m_positions.push_back(m_particles[i].position);
    }
}

// 清除所有球体碰撞约束
void Cloth::ClearSphereCollisionConstraints()
{
    // 从总约束列表中移除球体碰撞约束
    auto it = std::remove_if(m_constraints.begin(), m_constraints.end(), [this](Constraint* constraint) {
        return std::find(m_sphereConstraints.begin(), m_sphereConstraints.end(), constraint) != m_sphereConstraints.end();
    });
    m_constraints.erase(it, m_constraints.end());
    
    // 释放球体碰撞约束的内存
    for (auto constraint : m_sphereConstraints)
    {
        delete constraint;
    }
    m_sphereConstraints.clear();
}

// 创建布料的粒子
void Cloth::CreateParticles(float size, float mass)
{
    m_particles.reserve(m_width * m_height);
    
    float stepX = size / (m_width - 1);
    float stepZ = size / (m_height - 1);
    
    // 将位置转换为XMVECTOR进行计算
    dx::XMVECTOR posVector = dx::XMLoadFloat3(&dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
    
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            // 计算粒子位置
            dx::XMVECTOR offset = dx::XMVectorSet(x * stepX, 0.0f, y * stepZ, 0.0f);
            dx::XMVECTOR pos = dx::XMVectorAdd(posVector, offset);
            dx::XMFLOAT3 posFloat3;
            dx::XMStoreFloat3(&posFloat3, pos);
            
            // 左上角和右上角的粒子设为静态
            bool isStatic = (x == 0 && y == 0) || (x == m_width - 1 && y == 0);
            
            // 创建粒子
            m_particles.emplace_back(posFloat3, mass, isStatic);
        }
    }

    ComputeNormals();
}

// 创建布料的约束
void Cloth::CreateConstraints(float size)
{
    m_distanceConstraints.clear();
    m_constraints.clear();
    
    float restLength = size / (m_width - 1);
    float compliance = 1e-7f; // 减小柔度值以增加布料刚性
    
    // 创建结构约束（相邻粒子之间）
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            // 水平约束
            if (x < m_width - 1)
            {
                int idx1 = y * m_width + x;
                int idx2 = y * m_width + (x + 1);
                m_distanceConstraints.emplace_back(&m_particles[idx1], &m_particles[idx2], restLength, compliance);
            }
            
            // 垂直约束
            if (y < m_height - 1)
            {
                int idx1 = y * m_width + x;
                int idx2 = (y + 1) * m_width + x;
                m_distanceConstraints.emplace_back(&m_particles[idx1], &m_particles[idx2], restLength, compliance);
            }
            
            // 对角约束（可选，增加稳定性）
            if (x < m_width - 1 && y < m_height - 1)
            {
                int idx1 = y * m_width + x;
                int idx2 = (y + 1) * m_width + (x + 1);
                m_distanceConstraints.emplace_back(&m_particles[idx1], &m_particles[idx2], restLength * std::sqrt(2.0f), compliance);
            }
            
            if (x < m_width - 1 && y > 0)
            {
                int idx1 = y * m_width + x;
                int idx2 = (y - 1) * m_width + (x + 1);
                m_distanceConstraints.emplace_back(&m_particles[idx1], &m_particles[idx2], restLength * std::sqrt(2.0f), compliance);
            }
        }
    }
    
    // 填充约束指针数组
    for (auto& constraint : m_distanceConstraints)
    {
        m_constraints.push_back(&constraint);
    }
}