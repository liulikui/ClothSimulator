#include "Cloth.h"
#include <iostream>
#include <DirectXMath.h>
#include <algorithm>
#include "SphereCollisionConstraint.h"

extern void logDebug(const std::string& message);

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

Cloth::Cloth(int widthResolution, int heightResolution, float size, float mass, 
    ClothParticleMassMode massMode, ClothMeshAndContraintMode meshAndContraintMode)
    : Mesh()
    , m_widthResolution(widthResolution)
    , m_heightResolution(heightResolution)
    , m_size(size)
    , m_mass(mass)
    , m_massMode(massMode)
    , m_meshAndContraintMode(meshAndContraintMode)
    , m_distanceConstraintCompliance(1e-8f)
    , m_distanceConstraintDamping(1e-2f)
    , m_addDiagonalConstraints(true)
    , m_addBendingConstraints(true)
    , m_bendingConstraintCompliance(1e-5f)
    , m_bendingConstraintDamping(1e-3f)
    , m_addDihedralBendingConstraints(false)
    , m_dihedralBendingConstraintCompliance(1e-8f)
    , m_dihedralBendingConstraintDamping(1e-2f)
    , m_addLRAConstraints(true)
    , m_LRAConstraintCompliance(1e-8f)
    , m_LRAConstraintDamping(1e-2f)
    , m_LRAMaxStrech(0.01f)
    , m_sphereCollisionConstraintCompliance(1e-9f)
    , m_sphereCollisionConstraintDamping(1e-2f)
    , m_iteratorCount(20)
    , m_subIteratorCount(1)
    , m_solver(this)
{
    // 设置重力为标准地球重力
    m_gravity = dx::XMFLOAT3(0.0f, -9.8f, 0.0f);
}

Cloth::~Cloth()
{
    // 清除球体碰撞约束
    ClearSphereCollisionConstraints();
}

bool Cloth::Initialize(IRALDevice* device)
{
    // 确保device不为空
    if (!device)
    {
        std::cerr << "Cloth::Initialize: device is null" << std::endl;
        return false;
    }

    if (m_meshAndContraintMode == ClothMeshAndContraintMode::Full)
    {
        // 创建完整结构的布料的粒子
        CreateFullStructuredParticles();

        // 创建完整结构的布料的约束
        CreateFullStructuredConstraints();
    }
    else
    {
        // 创建简化结构的布料的粒子
        CreateSimplifiedStructuredParticles();

        // 创建简化结构的布料的约束
        CreateSimplifiedStructuredConstraints();
    }

    char buffer[128];
    sprintf_s(buffer, "Particles:%d, DistanceConstraints:%d, LRAConstraints:%d, DihedralBendingConstraints:%d, CollisionConstraints:%d"
        , (int)m_particles.size()
        , (int)m_distanceConstraints.size()
        , (int)m_lraConstraints.size()
        , (int)m_dihedralBendingConstraints.size()
        , (int)m_CollisionConstraints.size());

    logDebug(buffer);

    return true;
}

void Cloth::OnSetupMesh(IRALDevice* device, PrimitiveMesh& mesh)
{
    // 生成顶点数据（位置和法线）
    std::vector<float> vertexData;
    vertexData.reserve(m_particles.size() * 6);

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
        true,
        vertexData.data(),
        L"ClothVB"
    );

    // 创建索引缓冲区
    size_t indexBufferSize = m_indices.size() * sizeof(uint32_t);
    mesh.indexBuffer = device->CreateIndexBuffer(
        m_indices.size(),
        true, // 32位索引
        true,
        m_indices.data(),
        L"ClothIB"
    );
}

void Cloth::OnUpdateMesh(IRALDevice* device, PrimitiveMesh& mesh)
{
    // 生成顶点数据（位置和法线）
    std::vector<float> vertexData;
    vertexData.reserve(m_particles.size() * 6);

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
                &particle, 
                relativeCenter, 
                sphereRadius, 
                m_sphereCollisionConstraintCompliance, 
                m_sphereCollisionConstraintDamping);
            m_CollisionConstraints.push_back(constraint);
        }
    }
}

void Cloth::Update(IRALGraphicsCommandList* commandList, float deltaTime)
{
    // 使用XPBD求解器更新布料状态
    m_solver.Step(deltaTime);
    
    // 计算布料的法线数据
    // 清空位置和法线向量
    m_positions.clear();
    m_normals.clear();
    
    if (m_meshAndContraintMode == ClothMeshAndContraintMode::Full)
    {
        ComputeFullStructuredNormals();
    }
    else
    {
        ComputeSimplifiedStructuredNormals();
    }
   
    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        m_positions.push_back(m_particles[i].position);
    }
}

void Cloth::ClearSphereCollisionConstraints()
{
    // 释放球体碰撞约束的内存
    for (auto constraint : m_CollisionConstraints)
    {
        delete constraint;
    }

    m_CollisionConstraints.clear();
}

void Cloth::CreateParticles()
{
    m_particles.reserve(m_widthResolution * m_widthResolution);

    float stepW = m_size / (m_widthResolution - 1);
    float stepH = m_size / (m_heightResolution - 1);

    // 将位置转换为XMVECTOR进行计算
    dx::XMFLOAT3 origin = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
    dx::XMVECTOR posVector = dx::XMLoadFloat3(&origin);

    int totalParticles = m_widthResolution * m_heightResolution;
    int totalNonStaticParticles = totalParticles - 2; // 减去两个静态粒子

    float mass = m_mass;

    if (m_massMode == ClothParticleMassMode::FixedTotalMass && totalNonStaticParticles > 0)
    {
        mass = m_mass / totalNonStaticParticles; // 平均分配质量
    }

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
            m_particles.emplace_back(posFloat3, mass, isStatic);

#ifdef DEBUG_SOLVER
            Particle& particle = m_particles.back();
            particle.coordW = w;
            particle.coordH = h;
#endif//DEBUG_SOLVER
        }
    }
}

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

void Cloth::AddDihedralBendingConstraint(const DihedralBendingConstraint& constraint)
{
#ifdef DEBUG_SOLVER
    const Particle** particles = constraint.GetParticles();
    char buffer[256];
    sprintf_s(buffer, "[DEBUG] P1_w:%d, P1_h:%d P2_w:%d, P2_h:%d P3_w:%d, P3_h:%d P4_w:%d, P4_h:%d"
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

void Cloth::CreateFullStructuredParticles()
{
    CreateParticles();

    // 生成三角形面的索引数据
    for (int h = 0; h < m_heightResolution - 1; ++h)
    {
        for (int w = 0; w < m_widthResolution - 1; ++w)
        {
            int i1, i2, i3;

            // 第一个三角形：(w,h), (w+1,h+1), (w+1,h)
            i1 = (h) * m_widthResolution + (w);
            i2 = (h + 1) * m_widthResolution + (w + 1);
            i3 = (h) * m_widthResolution + (w + 1); 

            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);

            // 第二个三角形：(w,h), (w,h+1), (w+1,h+1)
            i1 = (h) * m_widthResolution + (w);
            i2 = (h + 1) * m_widthResolution + (w); 
            i3 = (h + 1) * m_widthResolution + (w + 1);

            m_indices.push_back(i1);
            m_indices.push_back(i2);
            m_indices.push_back(i3);
        }
    }

    ComputeFullStructuredNormals();
}

void Cloth::CreateFullStructuredConstraints()
{
#ifdef DEBUG_SOLVER
    logDebug("[DEBUG] Begin adding distance constraints");
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
                AddDistanceConstraint(DistanceConstraint(
                    &m_particles[id1], 
                    &m_particles[id2], 
                    m_distanceConstraintCompliance, 
                    m_distanceConstraintDamping));
            }
            
            // 垂直约束
            if (h < m_heightResolution - 1)
            {
                int id1 = (h)*m_widthResolution + (w);
                int id2 = (h + 1) * m_widthResolution + (w);
                AddDistanceConstraint(DistanceConstraint(
                    &m_particles[id1], 
                    &m_particles[id2], 
                    m_distanceConstraintCompliance, 
                    m_distanceConstraintDamping));
            }
        }
    }

#ifdef DEBUG_SOLVER
    logDebug("[DEBUG] End adding distance constraints");
#endif//DEBUG_SOLVER

    // 对角约束（可选，增加稳定性）
    if (m_addDiagonalConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding diagonal distance constraints");
#endif//DEBUG_SOLVER

        // 按格子
        for (int h = 0; h < m_heightResolution - 1; ++h)
        {
            for (int w = 0; w < m_widthResolution - 1; ++w)
            {
                int id1, id2;
                id1 = (h) * m_widthResolution + (w);
                id2 = (h + 1) * m_widthResolution + (w + 1);
                AddDistanceConstraint(DistanceConstraint(
                    &m_particles[id1], 
                    &m_particles[id2], 
                    m_distanceConstraintCompliance, 
                    m_distanceConstraintDamping));

                id1 = (h) * m_widthResolution + (w + 1);
                id2 = (h + 1) * m_widthResolution + (w);
                AddDistanceConstraint(DistanceConstraint(
                    &m_particles[id1], 
                    &m_particles[id2], 
                    m_distanceConstraintCompliance, 
                    m_distanceConstraintDamping));
            }
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding diagonal distance constraints");
#endif//DEBUG_SOLVER
    }

    if (m_addBendingConstraints && m_heightResolution > 2 && m_widthResolution > 2)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding bending constraints");
#endif//DEBUG_SOLVER
        for (int h = 0; h < m_heightResolution; ++h)
        {
            for (int w = 0; w < m_widthResolution; ++w)
            {
                if (w + 2 < m_widthResolution)
                {
                    int id1 = (h) * m_widthResolution + (w);
                    int id2 = (h) * m_widthResolution + (w + 2);
                    AddDistanceConstraint(DistanceConstraint(
                        &m_particles[id1], 
                        &m_particles[id2], 
                        m_bendingConstraintCompliance, 
                        m_bendingConstraintDamping));
                }

                if (h + 2 < m_heightResolution)
                {
                    int id1 = (h) * m_widthResolution + (w);
                    int id2 = (h + 2) * m_widthResolution + (w);
                    AddDistanceConstraint(DistanceConstraint(
                        &m_particles[id1], 
                        &m_particles[id2], 
                        m_bendingConstraintCompliance, 
                        m_bendingConstraintDamping));
                }
            }
        }
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding bending constraints");
#endif//DEBUG_SOLVER
    }

    if (m_addLRAConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding LRA constraints");
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
            AddLRAConstraint(LRAConstraint(
                &m_particles[i], 
                m_particles[leftTopIndex].position, 
                distanceToLeftTop, 
                m_LRAConstraintCompliance, 
                m_LRAConstraintDamping, 
                m_LRAMaxStrech));

            // 计算到右上角静止粒子的欧几里德距离作为测地线距离
            dx::XMVECTOR rightTopPos = dx::XMLoadFloat3(&m_particles[rightTopIndex].position);
            diff = dx::XMVectorSubtract(pos, rightTopPos);
            float distanceToRightTop = dx::XMVectorGetX(dx::XMVector3Length(diff));

            // 添加到右上角静止粒子的LRA约束
            AddLRAConstraint(LRAConstraint(
                &m_particles[i], 
                m_particles[rightTopIndex].position, 
                distanceToRightTop, 
                m_LRAConstraintCompliance, 
                m_LRAConstraintDamping, 
                m_LRAMaxStrech));
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding LRA constraints");
#endif//DEBUG_SOLVER
    }

    // 如果启用了二面角约束，则添加它们
    if (m_addDihedralBendingConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding dihedral bending constraints");
#endif//DEBUG_SOLVER
        
        // 遍历布料，为每对相邻的三角形创建二面角约束
        for (int h = 0; h < m_heightResolution - 1; ++h)
        {
            for (int w = 0; w < m_widthResolution - 1; ++w)
            {
                int p1, p2, p3, p4;

                // 公共顶点1 (w,h)
                p1 = (h) * m_widthResolution + (w);
                // 公共顶点2 (w+1,h+1)
                p2 = (h + 1) * m_widthResolution + (w + 1);
                // 第一个三角形的第三个顶点 (w+1,h)
                p3 = (h) * m_widthResolution + (w + 1);
                // 第二个三角形的第三个顶点 (w,h+1)
                p4 = (h + 1) * m_widthResolution + (w);

                AddDihedralBendingConstraint(DihedralBendingConstraint(
                    &m_particles[p1], // 公共顶点1
                    &m_particles[p2], // 公共顶点2
                    &m_particles[p3], // 三角形1的第三个顶点
                    &m_particles[p4], // 三角形2的第三个顶点
                    m_dihedralBendingConstraintCompliance,
                    m_dihedralBendingConstraintDamping));

                if (w + 2 < m_widthResolution)
                {
                    // 公共顶点1 (w+1,h)
                    p1 = (h) * m_widthResolution + (w + 1);
                    // 公共顶点2 (w+1,h+1)
                    p2 = (h + 1) * m_widthResolution + (w + 1);
                    // 第一个三角形的第三个顶点 (w,h)
                    p3 = (h) * m_widthResolution + (w);
                    // 第二个三角形的第三个顶点 (w+2,h+1)
                    p4 = (h + 1) * m_widthResolution + (w + 2);

                    AddDihedralBendingConstraint(DihedralBendingConstraint(
                        &m_particles[p1], // 公共顶点1
                        &m_particles[p2], // 公共顶点2
                        &m_particles[p3], // 三角形1的第三个顶点
                        &m_particles[p4], // 三角形2的第三个顶点
                        m_dihedralBendingConstraintCompliance,
                        m_dihedralBendingConstraintDamping));
                }
                
                if (h + 2 < m_heightResolution)
                {
                    // 公共顶点1 (w,h+1)
                    p1 = (h + 1) * m_widthResolution + (w);
                    // 公共顶点2 (w+1,h+1)
                    p2 = (h + 1) * m_widthResolution + (w + 1);
                    // 第一个三角形的第三个顶点 (w,h)
                    p3 = (h) * m_widthResolution + (w);
                    // 第二个三角形的第三个顶点 (w+1,h+2)
                    p4 = (h + 2) * m_widthResolution + (w + 1);

                    AddDihedralBendingConstraint(DihedralBendingConstraint(
                        &m_particles[p1], // 公共顶点1
                        &m_particles[p2], // 公共顶点2
                        &m_particles[p3], // 三角形1的第三个顶点
                        &m_particles[p4], // 三角形2的第三个顶点
                        m_dihedralBendingConstraintCompliance,
                        m_dihedralBendingConstraintDamping));
                }
            }
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding dihedral bending constraints");
#endif//DEBUG_SOLVER
    }
}

void Cloth::ComputeFullStructuredNormals()
{
    // 计算法线（使用面法线的平均值）
    std::vector<dx::XMFLOAT3> vertexNormals(m_particles.size(), dx::XMFLOAT3(0.0f, 0.0f, 0.0f));

    // 计算顶点法线-按格子
    for (int h = 0; h < m_heightResolution - 1; ++h)
    {
        for (int w = 0; w < m_widthResolution - 1; ++w)
        {
            // 第一个三角形：(w,h), (w+1,h+1), (w+1,h)
            int i1 = (h) * m_widthResolution + (w);
            int i2 = (h + 1) * m_widthResolution + (w + 1);
            int i3 = (h) * m_widthResolution + (w + 1);

            // 计算面法线
            dx::XMVECTOR v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), 
                dx::XMLoadFloat3(&m_particles[i1].position));
            dx::XMVECTOR v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), 
                dx::XMLoadFloat3(&m_particles[i1].position));
            dx::XMVECTOR crossProduct = dx::XMVector3Cross(v1, v2);

            dx::XMVECTOR normal = dx::XMVector3Normalize(crossProduct);

            // 累加到顶点法线
            dx::XMVECTOR n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
            dx::XMVECTOR n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
            dx::XMVECTOR n3 = dx::XMLoadFloat3(&vertexNormals[i3]);

            dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
            dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
            dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));

            // 第二个三角形：(w,h), (w,h+1), (w+1,h+1)
            i1 = (h) * m_widthResolution + (w);
            i2 = (h + 1) * m_widthResolution + (w);
            i3 = (h + 1) * m_widthResolution + (w + 1);

            // 计算面法线
            v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), 
                dx::XMLoadFloat3(&m_particles[i1].position));
            v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), 
                dx::XMLoadFloat3(&m_particles[i1].position));
            crossProduct = dx::XMVector3Cross(v1, v2);

            normal = dx::XMVector3Normalize(crossProduct);

            // 累加到顶点法线
            n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
            n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
            n3 = dx::XMLoadFloat3(&vertexNormals[i3]);

            dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
            dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
            dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
        }
    }

    m_normals.clear();
    m_normals.reserve(m_particles.size());

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

void Cloth::CreateSimplifiedStructuredParticles()
{
    CreateParticles();

    // 生成三角形面的索引数据
    for (int h = 0; h < m_heightResolution - 1; ++h)
    {
        for (int w = 0; w < m_widthResolution - 1; ++w)
        {
            int i1, i2, i3;

            if ((w + h) % 2 == 0)
            {
                // 第一个三角形：(w,h), (w+1,h+1), (w+1,h)
                i1 = (h) * m_widthResolution + (w);
                i2 = (h + 1) * m_widthResolution + (w + 1);
                i3 = (h) * m_widthResolution + (w + 1);

                m_indices.push_back(i1);
                m_indices.push_back(i2);
                m_indices.push_back(i3);

                // 第二个三角形：(w,h), (w,h+1), (w+1,h+1)
                i1 = (h) * m_widthResolution + (w);
                i2 = (h + 1) * m_widthResolution + (w);
                i3 = (h + 1) * m_widthResolution + (w + 1);

                m_indices.push_back(i1);
                m_indices.push_back(i2);
                m_indices.push_back(i3);
            }
            else
            {
                // 第一个三角形：(w,h), (w,h+1), (w+1,h)
                i1 = (h) * m_widthResolution + (w);
                i2 = (h + 1) * m_widthResolution + (w);
                i3 = (h) * m_widthResolution + (w + 1);

                m_indices.push_back(i1);
                m_indices.push_back(i2);
                m_indices.push_back(i3);

                // 第二个三角形：(w+1,h), (w,h+1), (w+1,h+1)
                i1 = (h) * m_widthResolution + (w + 1);
                i2 = (h + 1) * m_widthResolution + (w);
                i3 = (h + 1) * m_widthResolution + (w + 1);

                m_indices.push_back(i1);
                m_indices.push_back(i2);
                m_indices.push_back(i3);
            }
        }
    }

    ComputeSimplifiedStructuredNormals();
}

void Cloth::CreateSimplifiedStructuredConstraints()
{
#ifdef DEBUG_SOLVER
    logDebug("[DEBUG] Begin adding distance constraints");
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
                AddDistanceConstraint(DistanceConstraint(
                    &m_particles[id1], 
                    &m_particles[id2], 
                    m_distanceConstraintCompliance, 
                    m_distanceConstraintDamping));
            }

            // 垂直约束
            if (h < m_heightResolution - 1)
            {
                int id1 = (h)*m_widthResolution + (w);
                int id2 = (h + 1) * m_widthResolution + (w);
                AddDistanceConstraint(DistanceConstraint(
                    &m_particles[id1], 
                    &m_particles[id2], 
                    m_distanceConstraintCompliance, 
                    m_distanceConstraintDamping));
            }
        }
    }

#ifdef DEBUG_SOLVER
    logDebug("[DEBUG] End adding distance constraints");
#endif//DEBUG_SOLVER

    // 对角约束（可选，增加稳定性）
    if (m_addDiagonalConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding diagonal distance constraints");
#endif//DEBUG_SOLVER

        // 仅增加一个对角约束
        for (int h = 0; h < m_heightResolution - 1; ++h)
        {
            for (int w = 0; w < m_widthResolution - 1; ++w)
            {
                if ((w + h) % 2 == 1) // 注意这里和法线的计算的区别
                {
                    int id1 = (h) * m_widthResolution + (w);
                    int id2 = (h + 1) * m_widthResolution + (w + 1);
                    AddDistanceConstraint(DistanceConstraint(
                        &m_particles[id1], 
                        &m_particles[id2], 
                        m_distanceConstraintCompliance, 
                        m_distanceConstraintDamping));
                }
                else
                {
                    int id1 = (h) * m_widthResolution + (w + 1);
                    int id2 = (h + 1) * m_widthResolution + (w);
                    AddDistanceConstraint(DistanceConstraint(
                        &m_particles[id1], 
                        &m_particles[id2], 
                        m_distanceConstraintCompliance, 
                        m_distanceConstraintDamping));
                }
            }
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding diagonal distance constraints");
#endif//DEBUG_SOLVER
    }

    if (m_addBendingConstraints && m_heightResolution > 2 && m_widthResolution > 2)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding bending constraints");
#endif//DEBUG_SOLVER
        for (int h = 0; h < m_heightResolution; ++h)
        {
            for (int w = 0; w < m_widthResolution; ++w)
            {
                if (w + 2 < m_widthResolution)
                {
                    int id1 = (h) * m_widthResolution + (w);
                    int id2 = (h) * m_widthResolution + (w + 2);
                    AddDistanceConstraint(DistanceConstraint(
                        &m_particles[id1], 
                        &m_particles[id2], 
                        m_bendingConstraintCompliance, 
                        m_bendingConstraintDamping));
                }

                if (h + 2 < m_heightResolution)
                {
                    int id1 = (h) * m_widthResolution + (w);
                    int id2 = (h + 2) * m_widthResolution + (w);
                    AddDistanceConstraint(DistanceConstraint(
                        &m_particles[id1], 
                        &m_particles[id2], 
                        m_bendingConstraintCompliance, 
                        m_bendingConstraintDamping));
                }
            }
        }
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding bending constraints");
#endif//DEBUG_SOLVER
    }

    if (m_addLRAConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding LRA constraints");
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
            AddLRAConstraint(LRAConstraint(
                &m_particles[i], 
                m_particles[leftTopIndex].position, 
                distanceToLeftTop, 
                m_LRAConstraintCompliance, 
                m_LRAConstraintDamping, 
                m_LRAMaxStrech));

            // 计算到右上角静止粒子的欧几里德距离作为测地线距离
            dx::XMVECTOR rightTopPos = dx::XMLoadFloat3(&m_particles[rightTopIndex].position);
            diff = dx::XMVectorSubtract(pos, rightTopPos);
            float distanceToRightTop = dx::XMVectorGetX(dx::XMVector3Length(diff));

            // 添加到右上角静止粒子的LRA约束
            AddLRAConstraint(LRAConstraint(
                &m_particles[i], 
                m_particles[rightTopIndex].position, 
                distanceToRightTop, 
                m_LRAConstraintCompliance, 
                m_LRAConstraintDamping, 
                m_LRAMaxStrech));
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding LRA constraints");
#endif//DEBUG_SOLVER
    }

    // 如果启用了二面角约束，则添加它们
    if (m_addDihedralBendingConstraints)
    {
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] Begin adding dihedral bending constraints");
#endif//DEBUG_SOLVER

        // 遍历布料，为每对相邻的三角形创建二面角约束
        for (int h = 0; h < m_heightResolution - 1; ++h)
        {
            for (int w = 0; w < m_widthResolution - 1; ++w)
            {
                int p1, p2, p3, p4;

                if ((w + h) % 2 == 0)
                {
                    // 公共顶点1 (w,h)
                    p1 = (h) * m_widthResolution + (w);
                    // 公共顶点2 (w+1,h+1)
                    p2 = (h + 1) * m_widthResolution + (w + 1);
                    // 第一个三角形的第三个顶点 (w+1,h)
                    p3 = (h) * m_widthResolution + (w + 1);
                    // 第二个三角形的第三个顶点 (w,h+1)
                    p4 = (h + 1) * m_widthResolution + (w);

                    AddDihedralBendingConstraint(DihedralBendingConstraint(
                        &m_particles[p1], // 公共顶点1
                        &m_particles[p2], // 公共顶点2
                        &m_particles[p3], // 三角形1的第三个顶点
                        &m_particles[p4], // 三角形2的第三个顶点
                        m_dihedralBendingConstraintCompliance,
                        m_dihedralBendingConstraintDamping));

                    if (w + 2 < m_widthResolution)
                    {
                        // 公共顶点1 (w+1,h)
                        p1 = (h) * m_widthResolution + (w + 1);
                        // 公共顶点2 (w+1,h+1)
                        p2 = (h + 1) * m_widthResolution + (w + 1);
                        // 第一个三角形的第三个顶点 (w,h)
                        p3 = (h) * m_widthResolution + (w);
                        // 第二个三角形的第三个顶点 (w+2,h)
                        p4 = (h) * m_widthResolution + (w + 2);

                        AddDihedralBendingConstraint(DihedralBendingConstraint(
                            &m_particles[p1], // 公共顶点1
                            &m_particles[p2], // 公共顶点2
                            &m_particles[p3], // 三角形1的第三个顶点
                            &m_particles[p4], // 三角形2的第三个顶点
                            m_dihedralBendingConstraintCompliance,
                            m_dihedralBendingConstraintDamping));
                    }

                    if (h + 2 < m_heightResolution)
                    {
                        // 公共顶点1 (w,h+1)
                        p1 = (h + 1) * m_widthResolution + (w);
                        // 公共顶点2 (w+1,h+1)
                        p2 = (h + 1) * m_widthResolution + (w + 1);
                        // 第一个三角形的第三个顶点 (w,h)
                        p3 = (h)*m_widthResolution + (w);
                        // 第二个三角形的第三个顶点 (w,h+2)
                        p4 = (h + 2) * m_widthResolution + (w);

                        AddDihedralBendingConstraint(DihedralBendingConstraint(
                            &m_particles[p1], // 公共顶点1
                            &m_particles[p2], // 公共顶点2
                            &m_particles[p3], // 三角形1的第三个顶点
                            &m_particles[p4], // 三角形2的第三个顶点
                            m_dihedralBendingConstraintCompliance,
                            m_dihedralBendingConstraintDamping));
                    }
                }
                else
                {
                    // 公共顶点1 (w,h+1)
                    p1 = (h + 1) * m_widthResolution + (w);
                    // 公共顶点2 (w+1,h)
                    p2 = (h) * m_widthResolution + (w + 1);
                    // 第一个三角形的第三个顶点 (w,h)
                    p3 = (h) * m_widthResolution + (w);
                    // 第二个三角形的第三个顶点 (w+1,h+1)
                    p4 = (h + 1) * m_widthResolution + (w + 1);

                    if (w + 2 < m_widthResolution)
                    {
                        // 公共顶点1 (w+1,h)
                        p1 = (h) * m_widthResolution + (w + 1);
                        // 公共顶点2 (w+1,h+1)
                        p2 = (h + 1) * m_widthResolution + (w + 1);
                        // 第一个三角形的第三个顶点 (w,h+1)
                        p3 = (h + 1) * m_widthResolution + (w);
                        // 第二个三角形的第三个顶点 (w+2,h+1)
                        p4 = (h + 1) * m_widthResolution + (w + 2);

                        AddDihedralBendingConstraint(DihedralBendingConstraint(
                            &m_particles[p1], // 公共顶点1
                            &m_particles[p2], // 公共顶点2
                            &m_particles[p3], // 三角形1的第三个顶点
                            &m_particles[p4], // 三角形2的第三个顶点
                            m_dihedralBendingConstraintCompliance,
                            m_dihedralBendingConstraintDamping));
                    }

                    if (h + 2 < m_heightResolution)
                    {
                        // 公共顶点1 (w,h+1)
                        p1 = (h + 1) * m_widthResolution + (w);
                        // 公共顶点2 (w+1,h+1)
                        p2 = (h + 1) * m_widthResolution + (w + 1);
                        // 第一个三角形的第三个顶点 (w+1,h)
                        p3 = (h) * m_widthResolution + (w + 1);
                        // 第二个三角形的第三个顶点 (w+1,h+2)
                        p4 = (h + 2) * m_widthResolution + (w + 1);

                        AddDihedralBendingConstraint(DihedralBendingConstraint(
                            &m_particles[p1], // 公共顶点1
                            &m_particles[p2], // 公共顶点2
                            &m_particles[p3], // 三角形1的第三个顶点
                            &m_particles[p4], // 三角形2的第三个顶点
                            m_dihedralBendingConstraintCompliance,
                            m_dihedralBendingConstraintDamping));
                    }
                }
            }
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] End adding dihedral bending constraints");
#endif//DEBUG_SOLVER
    }
}

void Cloth::ComputeSimplifiedStructuredNormals()
{
    // 计算法线（使用面法线的平均值）
    std::vector<dx::XMFLOAT3> vertexNormals(m_particles.size(), dx::XMFLOAT3(0.0f, 0.0f, 0.0f));

    // 计算顶点法线
    for (int h = 0; h < m_heightResolution - 1; ++h)
    {
        for (int w = 0; w < m_widthResolution - 1; ++w)
        {
            int i1, i2, i3;

            if ((w + h) % 2 == 0)
            {
                // 第一个三角形：(w,h), (w+1,h+1), (w+1,h)
                i1 = (h) * m_widthResolution + (w);
                i2 = (h + 1) * m_widthResolution + (w + 1);
                i3 = (h) * m_widthResolution + (w + 1);

                // 计算面法线
                dx::XMVECTOR v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                dx::XMVECTOR v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                dx::XMVECTOR crossProduct = dx::XMVector3Cross(v1, v2);

                dx::XMVECTOR normal = dx::XMVector3Normalize(crossProduct);

                // 累加到顶点法线
                dx::XMVECTOR n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
                dx::XMVECTOR n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
                dx::XMVECTOR n3 = dx::XMLoadFloat3(&vertexNormals[i3]);

                dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
                dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
                dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));

                // 第二个三角形：(w,h), (w,h+1), (w+1,h+1)
                i1 = (h) * m_widthResolution + (w);
                i2 = (h + 1) * m_widthResolution + (w);
                i3 = (h + 1) * m_widthResolution + (w + 1);

                // 计算面法线
                v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                crossProduct = dx::XMVector3Cross(v1, v2);

                normal = dx::XMVector3Normalize(crossProduct);

                // 累加到顶点法线
                n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
                n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
                n3 = dx::XMLoadFloat3(&vertexNormals[i3]);

                dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
                dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
                dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
            }
            else
            {
                // 第一个三角形：(w,h), (w,h+1), (w+1,h)
                i1 = (h) * m_widthResolution + (w);
                i2 = (h + 1) * m_widthResolution + (w);
                i3 = (h) * m_widthResolution + (w + 1);

                // 计算面法线
                dx::XMVECTOR v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                dx::XMVECTOR v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                dx::XMVECTOR crossProduct = dx::XMVector3Cross(v1, v2);

                dx::XMVECTOR normal = dx::XMVector3Normalize(crossProduct);

                // 累加到顶点法线
                dx::XMVECTOR n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
                dx::XMVECTOR n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
                dx::XMVECTOR n3 = dx::XMLoadFloat3(&vertexNormals[i3]);

                dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
                dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
                dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));

                // 第二个三角形：(w+1,h), (w,h+1), (w+1,h+1)
                i1 = (h) * m_widthResolution + (w + 1);
                i2 = (h + 1) * m_widthResolution + (w);
                i3 = (h + 1) * m_widthResolution + (w + 1);

                // 计算面法线
                v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i2].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particles[i3].position), 
                    dx::XMLoadFloat3(&m_particles[i1].position));
                crossProduct = dx::XMVector3Cross(v1, v2);

                normal = dx::XMVector3Normalize(crossProduct);

                // 累加到顶点法线
                n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
                n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
                n3 = dx::XMLoadFloat3(&vertexNormals[i3]);

                dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
                dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
                dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
            }
        }
    }

    m_normals.clear();
    m_normals.reserve(m_particles.size());

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