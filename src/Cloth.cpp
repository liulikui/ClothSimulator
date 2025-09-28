#include "Cloth.h"
#include <iostream>
#include <DirectXMath.h>
#include <algorithm>
#include "SphereCollisionConstraint.h"

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
    , m_distanceConstraintCompliance(0.00000001f)
    , m_LRAConstraintCompliance(0.00000001f)
    , m_dihedralBendingConstraintCompliance(0.00000001f)
    , m_addDiagonalConstraints(true)
    , m_addLRAConstraints(true)
    , m_addBendingConstraints(false)
    , m_LRAMaxStrech(0.01f)
    , m_iteratorCount(20)
    , m_subIteratorCount(1)
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
            // 第一个三角形：(w,h), (w+1,h+1), (w+1,h)
            int i1 = h * m_widthResolution + w;
            int i2 = (h + 1) * m_widthResolution + w + 1;
            int i3 = h * m_widthResolution + w + 1; 

            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);

            // 第二个三角形：(w,h), (w,h+1), (w+1,h+1)
            i1 = h * m_widthResolution + w;
            i2 = (h + 1) * m_widthResolution + w; 
            i3 = (h + 1) * m_widthResolution + w + 1;

            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);
        }
    }

    ComputeNormals();
}

// 增加距离约束
void Cloth::AddDistanceConstraint(const DistanceConstraint& constraint)
{
#ifdef DEBUG_SOLVER
    const Particle** particles = constraint.GetParticles();
    char buffer[128];
    sprintf_s(buffer, "[DEBUG] P1_w:%d, P1_h:%d P2_w:%d, P2_h:%d"
        , particles[0]->coordW
        , particles[0]->coordH
        , particles[1]->coordW
        , particles[1]->coordH);
    logDebug(buffer);
#endif//DEBUG_SOLVER

    m_distanceConstraints.push_back(constraint);
}

// 增加LRA约束
void Cloth::AddLRAConstraint(const LRAConstraint& constraint)
{
#ifdef DEBUG_SOLVER
    const Particle** particles = constraint.GetParticles();
    char buffer[128];
    sprintf_s(buffer, "[DEBUG] P1_w:%d, P1_h:%d"
        , particles[0]->coordW
        , particles[0]->coordH);
    logDebug(buffer);
#endif//DEBUG_SOLVER

    m_lraConstraints.push_back(constraint);
}

// 增加二面角约束
void Cloth::AddDihedralBendingConstraint(const DihedralBendingConstraint& constraint)
{
#ifdef DEBUG_SOLVER
    const Particle** particles = constraint.GetParticles();
    char buffer[256];
    sprintf_s(buffer, "[DEBUG] P0_w:%d, P0_h:%d P1_w:%d, P1_h:%d P2_w:%d, P2_h:%d P3_w:%d, P3_h:%d"
        , particles[0]->coordW
        , particles[0]->coordH
        , particles[1]->coordW
        , particles[1]->coordH
        , particles[2]->coordW
        , particles[2]->coordH
        , particles[3]->coordW
        , particles[3]->coordH
    );
    logDebug(buffer);
#endif//DEBUG_SOLVER

    m_dihedralBendingConstraints.push_back(constraint);
}

// 创建布料的约束
void Cloth::CreateConstraints()
{
    m_distanceConstraints.clear();
    
    float restLength = m_size / (m_widthResolution - 1);

#ifdef DEBUG_SOLVER
    logDebug("[DEBUG] Begin Add Distance Constraints");
#endif//DEBUG_SOLVER

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
                AddDistanceConstraint(DistanceConstraint(&m_particles[id1], &m_particles[id2], restLength, m_distanceConstraintCompliance));
            }
            
            // 垂直约束
            if (h < m_heightResolution - 1)
            {
                int id1 = h * m_widthResolution + w;
                int id2 = (h + 1) * m_widthResolution + w;
                AddDistanceConstraint(DistanceConstraint(&m_particles[id1], &m_particles[id2], restLength, m_distanceConstraintCompliance));
            }
            
            if (m_addDiagonalConstraints)
            {
                // 对角约束（可选，增加稳定性）
                if (w < m_widthResolution - 1 && h < m_heightResolution - 1)
                {
                    int id1 = h * m_widthResolution + w;
                    int id2 = (h + 1) * m_widthResolution + (w + 1);
                    AddDistanceConstraint(DistanceConstraint(&m_particles[id1], &m_particles[id2], restLength * std::sqrt(2.0f), m_distanceConstraintCompliance));
                }

                if (w < m_widthResolution - 1 && h > 0)
                {
                    int id1 = h * m_widthResolution + w;
                    int id2 = (h - 1) * m_widthResolution + (w + 1);
                    AddDistanceConstraint(DistanceConstraint(&m_particles[id1], &m_particles[id2], restLength * std::sqrt(2.0f), m_distanceConstraintCompliance));
                }
            }
        }
    }
#ifdef DEBUG_SOLVER
    logDebug("[DEBUG] End Add Distance Constraints");
#endif//DEBUG_SOLVER

    if (m_addLRAConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin Add LRA Constraints");
#endif//DEBUG_SOLVER

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
            AddLRAConstraint(LRAConstraint(&m_particles[i], m_particles[leftTopIndex].position, distanceToLeftTop, m_LRAConstraintCompliance, m_LRAMaxStrech));

            // 计算到右上角静止粒子的欧几里德距离作为测地线距离
            dx::XMVECTOR rightTopPos = dx::XMLoadFloat3(&m_particles[rightTopIndex].position);
            diff = dx::XMVectorSubtract(pos, rightTopPos);
            float distanceToRightTop = dx::XMVectorGetX(dx::XMVector3Length(diff));

            // 添加到右上角静止粒子的LRA约束
            AddLRAConstraint(LRAConstraint(&m_particles[i], m_particles[rightTopIndex].position, distanceToRightTop, m_LRAConstraintCompliance, m_LRAMaxStrech));
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End Add LRA Constraints");
#endif//DEBUG_SOLVER
    }

    // 如果启用了二面角约束，则添加它们
    if (m_addBendingConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin Add Bending Constraints");
#endif//DEBUG_SOLVER

        m_dihedralBendingConstraints.clear();
        float restDihedralAngle = dx::XM_PI;            // 初始静止二面角设为XM_PI（平面）
        
        // 遍历布料，为每对相邻的三角形创建二面角约束
        
        // 1. 处理水平方向的边，为每对水平相邻的三角形创建约束
        // 为3*3网格示例：
        // p1(1,0), p2(1,1), p3(0,0), p4(2,1)
        for (int h = 0; h < m_heightResolution - 1; ++h)
        {
            for (int w = 1; w + 1 < m_widthResolution; ++w)
            {
                // 公共顶点1 (w,h)
                int p1 = h * m_widthResolution + w;
                // 公共顶点2 (w,h+1)
                int p2 = (h + 1) * m_widthResolution + w;
                // 第一个三角形的第三个顶点 (w-1,h)
                int p3 = h * m_widthResolution + (w - 1);
                // 第二个三角形的第三个顶点 (w+1,w+1)
                int p4 = (h + 1) * m_widthResolution + (w + 1);

                AddDihedralBendingConstraint(DihedralBendingConstraint(
                    &m_particles[p1], // 公共顶点1
                    &m_particles[p2], // 公共顶点2
                    &m_particles[p3], // 三角形1的第三个顶点
                    &m_particles[p4], // 三角形2的第三个顶点
                    restDihedralAngle, 
                    m_dihedralBendingConstraintCompliance));
            }
        }
        
        //// 2. 处理垂直方向的边，为每对垂直相邻的三角形创建约束
        //// 为4*4网格示例：
        //// p1(0,1), p2(1,1), p3(0,0), p4(1,2)
        //for (int w = 0; w < m_widthResolution - 1; ++w)
        //{
        //    for (int h = 0; h + 1 < m_heightResolution - 1; ++h)
        //    {
        //        // 公共顶点1 (w,h+１)
        //        int p1 = (h + 1) * m_widthResolution + w;
        //        // 公共顶点2 (w+1,h+1)
        //        int p2 = (h + 1) * m_widthResolution + (w + 1);
        //        // 第一个三角形的第三个顶点 (w,h)
        //        int p3 = h * m_widthResolution + w;
        //        // 第二个三角形的第三个顶点 (w+1,h+2)
        //        int p4 = (h + 2) * m_widthResolution + (w + 1);
        //        
        //        AddDihedralBendingConstraint(DihedralBendingConstraint(
        //            &m_particles[p1], // 公共顶点1
        //            &m_particles[p2], // 公共顶点2
        //            &m_particles[p3], // 三角形1的第三个顶点
        //            &m_particles[p4], // 三角形2的第三个顶点
        //            restDihedralAngle, 
        //            m_dihedralBendingConstraintCompliance));
        //    }
        //}

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End Add Bending Constraints");
#endif//DEBUG_SOLVER
    }
}