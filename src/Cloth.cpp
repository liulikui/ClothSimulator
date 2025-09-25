#include "Cloth.h"
#include <iostream>
#include <DirectXMath.h>
#include <algorithm>
#include "SphereCollisionConstraint.h"
#include "DX12RALResource.h"

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 构造函数：创建一个布料对象
// 参数：
//   widthResolution - 布料宽度方向的粒子数
//   heightResolution - 布料高度方向的粒子数
//   size - 布料的实际物理尺寸（以米为单位）
//   mass - 每个粒子的质量
Cloth::Cloth(int widthResolution, int heightResolution, float size, float mass)
    : Mesh()
    , m_widthResolution(widthResolution)
    , m_heightResolution(heightResolution)
    , m_size(size)
    , m_mass(mass)
    , m_useXPBDCollision(true)
    , m_addLRAConstraint(true)
    , m_addBendingConstraints(false)
    , m_LRAMaxStrech(0.01f)
    , m_iteratorCount(10)
    , m_subIteratorCount(4)
    , m_solver(this)
{
    // 设置重力为标准地球重力
    m_gravity = dx::XMFLOAT3(0.0f, -9.8f, 0.0f);
}

// 析构函数
Cloth::~Cloth()
{
    // 清除球体碰撞约束
    ClearSphereCollisionConstraints();
}

// 初始化布料的顶点和索引缓冲区
bool Cloth::Initialize(IRALDevice* device)
{
    // 确保device不为空
    if (!device)
    {
        std::cerr << "Cloth::Initialize: device is null" << std::endl;
        return false;
    }

    // 创建粒子
    CreateParticles();

    // 创建布料的约束
    CreateConstraints();

    return true;
}

void Cloth::OnSetupMesh(IRALDevice* device, PrimitiveMesh& mesh)
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
    mesh.vertexBuffer = device->CreateVertexBuffer(
        vertexBufferSize,
        6 * sizeof(float),// 顶点 stride（3个位置分量 + 3个法线分量）
        true
    );

    // 创建索引缓冲区
    size_t indexBufferSize = m_indices.size() * sizeof(uint32_t);
    mesh.indexBuffer = device->CreateIndexBuffer(
        m_indices.size(),
        true, // 32位索引
        true
    );

    // 上传顶点数据
    device->UploadBuffer(mesh.vertexBuffer.Get(), (const char*)vertexData.data(), vertexBufferSize);

    // 上传索引数据
    device->UploadBuffer(mesh.indexBuffer.Get(), (const char*)m_indices.data(), indexBufferSize);
}

void Cloth::OnUpdateMesh(IRALDevice* device, PrimitiveMesh& mesh)
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
    device->UploadBuffer(mesh.vertexBuffer.Get(), (const char*)vertexData.data(), vertexBufferSize);
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
            m_CollisionConstraints.push_back(constraint);
        }
    }
}

void Cloth::ComputeNormals()
{
    m_normals.clear();

    // 计算法线（使用面法线的平均值）
    std::vector<dx::XMFLOAT3> vertexNormals(m_particles.size(), dx::XMFLOAT3(0.0f, 0.0f, 0.0f));

    // 计算顶点法线
    for (int h = 0; h < m_heightResolution - 1; ++h)
    {
        for (int w = 0; w < m_widthResolution - 1; ++w)
        {
            // 第一个三角形：(w,h), (w+1,h), (w+1,h+1)
            int i1 = h * m_widthResolution + w;
            int i2 = h * m_widthResolution + w + 1;
            int i3 = (h + 1) * m_widthResolution + w + 1;

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

            // 第二个三角形：(w,h), (w+1,h+1), (w,h+1)
            i1 = h * m_widthResolution + w;
            i2 = (h + 1) * m_widthResolution + w + 1;
            i3 = (h + 1) * m_widthResolution + w;

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

        if (lengthSquared > 0.0001f)
        {
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
void Cloth::Update(IRALGraphicsCommandList* commandList, float deltaTime)
{
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
    // 释放球体碰撞约束的内存
    for (auto constraint : m_CollisionConstraints)
    {
        delete constraint;
    }

    m_CollisionConstraints.clear();
}

// 创建布料的粒子
void Cloth::CreateParticles()
{
    m_particles.reserve(m_widthResolution * m_widthResolution);
    
    float stepW = m_size / (m_widthResolution - 1);
    float stepH = m_size / (m_heightResolution - 1);
    
    // 将位置转换为XMVECTOR进行计算
    dx::XMFLOAT3 origin = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
    dx::XMVECTOR posVector = dx::XMLoadFloat3(&origin);
    
    for (int h = 0; h < m_heightResolution; ++h)
    {
        for (int w = 0; w < m_widthResolution; ++w)
        {
            // 计算粒子位置
            dx::XMVECTOR offset = dx::XMVectorSet(w * stepW, 0.0f, h * stepH, 0.0f);
            dx::XMVECTOR pos = dx::XMVectorAdd(posVector, offset);
            dx::XMFLOAT3 posFloat3;
            dx::XMStoreFloat3(&posFloat3, pos);
            
            // 左上角和右上角的粒子设为静态
            bool isStatic = (w == 0 && h == 0) || (w == m_widthResolution - 1 && h == 0);
            
            // 创建粒子
            m_particles.emplace_back(posFloat3, m_mass, isStatic);

#ifdef DEBUG_SOLVER
            Particle& particle = m_particles.back();
            particle.coordW = w;
            particle.coordH = h;
#endif//DEBUG_SOLVER
        }
    }

    // 生成三角形面的索引数据
    for (int h = 0; h < m_heightResolution - 1; ++h)
    {
        for (int w = 0; w < m_widthResolution - 1; ++w)
        {
            // 第一个三角形：(w,h), (w+1,h), (w+1,h+1)
            int i1 = h * m_widthResolution + w;
            int i2 = h * m_widthResolution + w + 1;
            int i3 = (h + 1) * m_widthResolution + w + 1;

            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);

            // 第二个三角形：(w,h), (w+1,h+1), (w,h+1)
            i1 = h * m_widthResolution + w;
            i2 = (h + 1) * m_widthResolution + w + 1;
            i3 = (h + 1) * m_widthResolution + w;

            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);
        }
    }

    ComputeNormals();
}

// 创建布料的约束
void Cloth::CreateConstraints()
{
    m_distanceConstraints.clear();
    
    float restLength = m_size / (m_widthResolution - 1);
    float compliance = 1e-7f; // 减小柔度值以增加布料刚性
    
    // 创建结构约束（相邻粒子之间）
    for (int h = 0; h < m_heightResolution; ++h)
    {
        for (int w = 0; w < m_widthResolution; ++w)
        {
            // 水平约束
            if (w < m_widthResolution - 1)
            {
                int id1 = h * m_widthResolution + w;
                int id2 = h * m_widthResolution + (w + 1);
                m_distanceConstraints.emplace_back(&m_particles[id1], &m_particles[id2], restLength, compliance);
            }
            
            // 垂直约束
            if (h < m_heightResolution - 1)
            {
                int id1 = h * m_widthResolution + w;
                int id2 = (h + 1) * m_widthResolution + w;
                m_distanceConstraints.emplace_back(&m_particles[id1], &m_particles[id2], restLength, compliance);
            }
            
            // 对角约束（可选，增加稳定性）
            if (w < m_widthResolution - 1 && h < m_heightResolution - 1)
            {
                int id1 = h * m_widthResolution + w;
                int id2 = (h + 1) * m_widthResolution + (h + 1);
                m_distanceConstraints.emplace_back(&m_particles[id1], &m_particles[id2], restLength * std::sqrt(2.0f), compliance);
            }
            
            if (w < m_widthResolution - 1 && h > 0)
            {
                int id1 = h * m_widthResolution + w;
                int id2 = (h - 1) * m_widthResolution + (w + 1);
                m_distanceConstraints.emplace_back(&m_particles[id1], &m_particles[id2], restLength * std::sqrt(2.0f), compliance);
            }
        }
    }

    if (m_addLRAConstraint)
    {
        // 为除静止粒子外的所有粒子添加LRA约束
        // 获取两个静止粒子的位置
        int leftTopIndex = 0; // 左上角静止粒子索引
        int rightTopIndex = m_widthResolution - 1; // 右上角静止粒子索引

        // 为每个非静止粒子添加到两个静止粒子的LRA约束
        for (int i = 0; i < m_particles.size(); ++i)
        {
            // 跳过静止粒子
            if (i == leftTopIndex || i == rightTopIndex || m_particles[i].isStatic)
                continue;

            // 计算到左上角静止粒子的欧几里德距离作为测地线距离
            dx::XMVECTOR pos = dx::XMLoadFloat3(&m_particles[i].position);
            dx::XMVECTOR leftTopPos = dx::XMLoadFloat3(&m_particles[leftTopIndex].position);
            dx::XMVECTOR diff = dx::XMVectorSubtract(pos, leftTopPos);
            float distanceToLeftTop = dx::XMVectorGetX(dx::XMVector3Length(diff));

            // 添加到左上角静止粒子的LRA约束
            m_lraConstraints.emplace_back(&m_particles[i], m_particles[leftTopIndex].position, distanceToLeftTop, compliance, m_LRAMaxStrech);

            // 计算到右上角静止粒子的欧几里德距离作为测地线距离
            dx::XMVECTOR rightTopPos = dx::XMLoadFloat3(&m_particles[rightTopIndex].position);
            diff = dx::XMVectorSubtract(pos, rightTopPos);
            float distanceToRightTop = dx::XMVectorGetX(dx::XMVector3Length(diff));

            // 添加到右上角静止粒子的LRA约束
            m_lraConstraints.emplace_back(&m_particles[i], m_particles[rightTopIndex].position, distanceToRightTop, compliance, m_LRAMaxStrech);
        }
    }

    // 如果启用了二面角约束，则添加它们
    if (m_addBendingConstraints)
    {
        m_dihedralBendingConstraints.clear();
        float bendingCompliance = compliance * 10.0f;   // 二面角约束使用稍大的柔度值
        float restDihedralAngle = dx::XM_PI;            // 初始静止二面角设为XM_PI（平面）
        
        // 遍历布料，为每对相邻的三角形创建二面角约束
        
        // 1. 处理水平方向的边，为每对水平相邻的三角形创建约束
        // 为4*4网格示例：
        // 第一个约束: p0(1,0), p1(1,1), p2(0,0), p3(2,1)
        // 第二个约束: p0(2,0), p1(2,1), p2(1,0), p3(3,1)
        for (int h = 0; h < m_heightResolution - 1; ++h)
        {
            for (int w = 1; w + 1 < m_widthResolution; ++w)
            {
                // 公共顶点1 (w,h)
                int p0 = h * m_widthResolution + w;
                // 公共顶点2 (w,h+1)
                int p1 = (h + 1) * m_widthResolution + w;
                // 第一个三角形的第三个顶点 (w-1,h)
                int p2 = h * m_widthResolution + (w - 1);
                // 第二个三角形的第三个顶点 (w+1,w+1)
                int p3 = (h + 1) * m_widthResolution + (w + 1);

                m_dihedralBendingConstraints.emplace_back(
                    &m_particles[p0],  // 公共顶点1
                    &m_particles[p1],  // 公共顶点2
                    &m_particles[p2],  // 三角形1的第三个顶点
                    &m_particles[p3],  // 三角形2的第三个顶点
                    restDihedralAngle,
                    bendingCompliance
                );
            }
        }
        
        //// 2. 处理垂直方向的边，为每对垂直相邻的三角形创建约束
        //// 参考水平方向约束的实现方式
        //for (int h = 1; h + 1 < m_heightResolution; ++h)
        //{
        //    for (int w = 0; w < m_widthResolution - 1; ++w)
        //    {
        //        // 公共顶点1 (w,h)
        //        int p0 = h * m_widthResolution + w;
        //        // 公共顶点2 (w+1,h)
        //        int p1 = h * m_widthResolution + (w + 1);
        //        // 第一个三角形的第三个顶点 (w,h-1)
        //        int p2 = (h - 1) * m_widthResolution + w;
        //        // 第二个三角形的第三个顶点 (w+1,h+1)
        //        int p3 = (h + 1) * m_widthResolution + (w + 1);
        //        
        //        // 对于4*4网格w=0列，h从1到2的约束顶点索引：
        //        // 第一个约束(h=1): p0(0,1), p1(1,1), p2(0,0), p3(1,2)
        //        // 第二个约束(h=2): p0(0,2), p1(1,2), p2(0,1), p3(1,3)
        //        
        //        m_dihedralBendingConstraints.emplace_back(
        //            &m_particles[p0],  // 公共顶点1
        //            &m_particles[p1],  // 公共顶点2
        //            &m_particles[p2],  // 三角形1的第三个顶点
        //            &m_particles[p3],  // 三角形2的第三个顶点
        //            restDihedralAngle,
        //            bendingCompliance
        //        );
        //    }
        //}
        
        //// 3. 处理主对角线方向的边（从左上到右下）
        //// 注意：这里只添加一种对角线方向的约束，避免重复约束导致过度刚硬
        //for (int h = 0; h < m_heightResolution - 1; ++h)
        //{
        //    for (int w = 0; w < m_widthResolution - 1; ++w)
        //    {
        //        // 两个三角形共享主对角线 (w,h)-(w+1,h+1)
        //        // 第一个三角形：(w,h), (w+1,h), (w+1,w+1)
        //        // 第二个三角形：(w,h), (w+1,h+1), (w,h+1)
        //        int p0 = h * m_widthResolution + w;            // 共享顶点1
        //        int p1 = (h + 1) * m_widthResolution + (w + 1); // 共享顶点2
        //        int p2 = h * m_widthResolution + (w + 1);      // 三角形1的第三个顶点
        //        int p3 = (h + 1) * m_widthResolution + w;      // 三角形2的第三个顶点
        //        
        //        // 创建二面角约束，使用与m_indices中一致的顶点顺序
        //        // 三角形1: (p0,p2,p1)
        //        // 三角形2: (p0,p1,p3)
        //        m_dihedralBendingConstraints.emplace_back(
        //            &m_particles[p0],  // 公共顶点1
        //            &m_particles[p1],  // 公共顶点2
        //            &m_particles[p2],  // 三角形1的第三个顶点
        //            &m_particles[p3],  // 三角形2的第三个顶点
        //            restDihedralAngle,
        //            bendingCompliance
        //        );
        //    }
        //}
    }
}