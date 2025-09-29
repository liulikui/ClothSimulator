#ifndef DIHEDRAL_BENDING_CONSTRAINT_H
#define DIHEDRAL_BENDING_CONSTRAINT_H

#include "Constraint.h"
#include <DirectXMath.h>
#include <cmath>  // 用于dx::XM_PI（需确保编译器支持，若不支持可手动定义#define dx::XM_PI 3.14159265358979323846）
#include <string>

// 命名空间别名简化使用
namespace dx = DirectX;

extern void logDebug(const std::string& message);

// 基于二面角的弯曲约束类，继承自约束基类
// 适配DirectX左手坐标系，严格遵循"二面角=180°-法向量夹角"的几何关系
class DihedralBendingConstraint : public Constraint
{
public:
    // 构造函数
    // 参数：
    //   p0, p1 - 两个三角形共享边的顶点（公共边为p0-p1）
    //   p2 - 第一个三角形的第三个顶点（三角形1：p0-p1-p2）
    //   p3 - 第二个三角形的第三个顶点（三角形2：p0-p1-p3）
    //   restDihedralAngle - 约束的静止二面角（弧度，范围[0, dx::XM_PI]，水平共面时设为dx::XM_PI）
    //   compliance - 约束的柔度
    DihedralBendingConstraint(Particle* p0, Particle* p1, Particle* p2, Particle* p3,
        float restDihedralAngle, float compliance, float damping)
        : Constraint(compliance, damping)
        , m_particle1(p0)
        , m_particle2(p1)
        , m_particle3(p2)
        , m_particle4(p3)
        , m_restDihedralAngle(restDihedralAngle)
    {
    }

    // 计算约束偏差
    // 返回：约束偏差值C = 当前二面角 - 静止二面角（二面角范围[0, dx::XM_PI]）
    float ComputeConstraintAndGradient(dx::XMFLOAT3* gradients) const override
    {
        dx::XMVECTOR n1 = ComputeTriangleNormal(m_particle1, m_particle2, m_particle3);
        dx::XMVECTOR n2 = ComputeTriangleNormal(m_particle1, m_particle2, m_particle4);

        // 法向量长度（用于归一化）
        float len1 = dx::XMVectorGetX(dx::XMVector3Length(n1));
        float len2 = dx::XMVectorGetX(dx::XMVector3Length(n2));

        // 避免法向量为零（三角形退化）
        if (len1 < 1e-6f || len2 < 1e-6f)
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[2] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            gradients[3] = dx::XMFLOAT3(0.0f, -1.0f, 0.0f);
            return 0.0f;
        }

        // 归一化法向量
        dx::XMVECTOR n1Norm = dx::XMVectorDivide(n1, dx::XMVectorReplicate(len1));
        dx::XMVECTOR n2Norm = dx::XMVectorDivide(n2, dx::XMVectorReplicate(len2));

        // 计算法向量点积（法向量夹角的余弦值）
        float dotNN = dx::XMVectorGetX(dx::XMVector3Dot(n1Norm, n2Norm));

        // 钳位避免acos输入越界（浮点误差导致）
        dotNN = dx::XMVectorClamp(dx::XMVectorReplicate(dotNN),
            dx::XMVectorReplicate(-1.0f),
            dx::XMVectorReplicate(1.0f)).m128_f32[0];

        // 计算法向量夹角（范围[0, M_PI]）
        float normalAngle = acosf(dotNN);

        // 公共边向量（p2 - p1）
        dx::XMVECTOR edge = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particle2->position), 
            dx::XMLoadFloat3(&m_particle1->position));

        dx::XMVECTOR crossNN = dx::XMVector3Normalize(dx::XMVector3Cross(n1Norm, n2Norm));
        float dir = dx::XMVectorGetX(dx::XMVector3Dot(crossNN, edge));

        float currentDihedralAngle;

        if (dir > 0.0f)
        {
            currentDihedralAngle = normalAngle;
        }
        else
        {
            currentDihedralAngle = dx::XM_2PI - normalAngle;
        }

#ifdef DEBUG_SOLVER
        char buffer[256];
        sprintf_s(buffer, "[DEBUG] dir:%f, normalAngle:%f currentDihedralAngle:%f", dir, normalAngle, currentDihedralAngle);
        logDebug(buffer);
#endif//DEBUG_SOLVER

        ComputeGradient(gradients);

        // 约束偏差：当前二面角 - 静止二面角
        return currentDihedralAngle - m_restDihedralAngle;
    }

    // 获取受此约束影响的粒子数量
    uint32_t GetParticlesCount() const override
    {
        return 4; // 二面角约束涉及4个顶点
    }

    // 获取受此约束影响的粒子数组
    Particle** GetParticles() override
    {
        return &m_particle1;
    }

    // 获取受此约束影响的所有粒子（const版本）
    virtual const Particle** GetParticles() const override
    {
        return (const Particle**)(&m_particle1);
    }

    // 设置约束的静止二面角
    void SetRestDihedralAngle(float angle)
    {
        // 确保静止角在[0, dx::XM_PI]范围内（避免无效值）
        m_restDihedralAngle = dx::XMVectorClamp(dx::XMVectorReplicate(angle),
            dx::XMVectorReplicate(0.0f),
            dx::XMVectorReplicate((float)dx::XM_PI)).m128_f32[0];
    }

    // 获取约束的静止二面角
    float GetRestDihedralAngle() const
    {
        return m_restDihedralAngle;
    }

    // 获取约束类型
    const char* GetConstraintType() const override
    {
        return "DihedralBending";
    }

private:
    // 计算约束梯度
    // 参数：
    //   gradients - 存储每个受约束粒子的梯度向量的数组（大小为4）
    void ComputeGradient(dx::XMFLOAT3* gradients) const
    {
        // 计算两个三角形的法向量（未归一化）及长度
        dx::XMVECTOR n1 = ComputeTriangleNormal(m_particle1, m_particle2, m_particle3);
        dx::XMVECTOR n2 = ComputeTriangleNormal(m_particle1, m_particle2, m_particle4);
        float len1 = dx::XMVectorGetX(dx::XMVector3Length(n1));
        float len2 = dx::XMVectorGetX(dx::XMVector3Length(n2));

        // 处理退化三角形（法向量为零）
        if (len1 < 1e-6f || len2 < 1e-6f)
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[2] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            gradients[3] = dx::XMFLOAT3(0.0f, -1.0f, 0.0f);
            return;
        }

        // 归一化法向量
        dx::XMVECTOR n1Norm = dx::XMVectorDivide(n1, dx::XMVectorReplicate(len1));
        dx::XMVECTOR n2Norm = dx::XMVectorDivide(n2, dx::XMVectorReplicate(len2));

        // 公共边向量（p2 - p1）
        dx::XMVECTOR e = dx::XMVectorSubtract(dx::XMLoadFloat3(&m_particle2->position),
            dx::XMLoadFloat3(&m_particle1->position));
        dx::XMVECTOR eNorm = dx::XMVector3Normalize(e);

        // 计算二面角的正弦值（用于梯度归一化，避免除零）
        dx::XMVECTOR crossNN = dx::XMVector3Cross(n1Norm, n2Norm);
        float sinTheta = dx::XMVectorGetX(dx::XMVector3Length(crossNN));
        sinTheta = (sinTheta < 1e-6f) ? 1e-6f : sinTheta;

        // 计算每个顶点对两个法向量的梯度贡献
        dx::XMVECTOR dn1_dp1, dn1_dp2, dn1_dp3;
        ComputeNormalGradients(m_particle1, m_particle2, m_particle3, n1, len1,
            dn1_dp1, dn1_dp2, dn1_dp3);

        dx::XMVECTOR dn2_dp1, dn2_dp2, dn2_dp4;
        ComputeNormalGradients(m_particle1, m_particle2, m_particle4, n2, len2,
            dn2_dp1, dn2_dp2, dn2_dp4);

        // 二面角对法向量的导数（链式法则中间量）
        dx::XMVECTOR dTheta_dn1 = dx::XMVectorMultiply(
            dx::XMVector3Cross(eNorm, n2Norm),
            dx::XMVectorReplicate(1.0f / sinTheta)
        );

        dx::XMVECTOR dTheta_dn2 = dx::XMVectorMultiply(
            dx::XMVector3Cross(n1Norm, eNorm),
            dx::XMVectorReplicate(1.0f / sinTheta)
        );

        // 计算原始梯度（基于法向量夹角推导）
        dx::XMVECTOR grad1Raw = dx::XMVectorAdd(
            dx::XMVector3Dot(dn1_dp1, dTheta_dn1),
            dx::XMVector3Dot(dn2_dp1, dTheta_dn2)
        );
        dx::XMVECTOR grad2Raw = dx::XMVectorAdd(
            dx::XMVector3Dot(dn1_dp2, dTheta_dn1),
            dx::XMVector3Dot(dn2_dp2, dTheta_dn2)
        );
        dx::XMVECTOR grad3Raw = dx::XMVector3Dot(dn1_dp3, dTheta_dn1);
        dx::XMVECTOR grad4Raw = dx::XMVector3Dot(dn2_dp4, dTheta_dn2);

        gradients[0] = XMVectorToFloat3(dx::XMVectorNegate(grad1Raw));
        gradients[1] = XMVectorToFloat3(dx::XMVectorNegate(grad2Raw));
        gradients[2] = XMVectorToFloat3(dx::XMVectorNegate(grad3Raw));
        gradients[3] = XMVectorToFloat3(dx::XMVectorNegate(grad4Raw));
    }

    // 辅助函数：计算三角形法向量
    dx::XMVECTOR ComputeTriangleNormal(Particle* a, Particle* b, Particle* c) const
    {
        dx::XMVECTOR posA = dx::XMLoadFloat3(&a->position);
        dx::XMVECTOR posB = dx::XMLoadFloat3(&b->position);
        dx::XMVECTOR posC = dx::XMLoadFloat3(&c->position);

        dx::XMVECTOR v1 = dx::XMVectorSubtract(posB, posA);  // 边AB
        dx::XMVECTOR v2 = dx::XMVectorSubtract(posC, posA);  // 边AC
        return dx::XMVector3Cross(v1, v2);
    }

    // 辅助函数：计算法向量对三个顶点的梯度（左手坐标系适配）
    void ComputeNormalGradients(Particle* a, Particle* b, Particle* c,
        dx::XMVECTOR normal, float normalLen,
        dx::XMVECTOR& dn_da, dx::XMVECTOR& dn_db, dx::XMVECTOR& dn_dc) const
    {
        dx::XMVECTOR posA = dx::XMLoadFloat3(&a->position);
        dx::XMVECTOR posB = dx::XMLoadFloat3(&b->position);
        dx::XMVECTOR posC = dx::XMLoadFloat3(&c->position);

        dx::XMVECTOR vAB = dx::XMVectorSubtract(posB, posA);
        dx::XMVECTOR vAC = dx::XMVectorSubtract(posC, posA);
        dx::XMVECTOR vBC = dx::XMVectorSubtract(posC, posB);

        dx::XMVECTOR dn_da_unnorm = dx::XMVector3Cross(vAC, vAB);
        dx::XMVECTOR dn_db_unnorm = dx::XMVector3Cross(vAB, vBC);
        dx::XMVECTOR dn_dc_unnorm = dx::XMVector3Cross(vBC, vAC);

        dx::XMVECTOR normalNorm = dx::XMVectorDivide(normal, dx::XMVectorReplicate(normalLen));

        // 归一化法向量的导数计算（向量微积分标准公式）
        auto normalize_derivative = [&](dx::XMVECTOR unnorm_deriv) {
            float dotProd = dx::XMVectorGetX(dx::XMVector3Dot(unnorm_deriv, normal));
            return dx::XMVectorDivide(
                dx::XMVectorSubtract(unnorm_deriv, dx::XMVectorMultiply(normal, dx::XMVectorReplicate(dotProd / normalLen))),
                dx::XMVectorReplicate(normalLen)
            );
            };

        dn_da = normalize_derivative(dn_da_unnorm);
        dn_db = normalize_derivative(dn_db_unnorm);
        dn_dc = normalize_derivative(dn_dc_unnorm);
    }

    // 辅助函数：将XMVECTOR转换为XMFLOAT3
    dx::XMFLOAT3 XMVectorToFloat3(dx::XMVECTOR vec) const
    {
        dx::XMFLOAT3 result;
        dx::XMStoreFloat3(&result, vec);
        return result;
    }

private:
    // 受约束的四个顶点（两个相邻三角形：(p0,p1,p2)和(p0,p1,p3)，共享边p0-p1）
    Particle* m_particle1;
    Particle* m_particle2;
    Particle* m_particle3;
    Particle* m_particle4;

    // 约束参数
    float m_restDihedralAngle;  // 静止二面角（弧度，范围[0, dx::XM_PI]）
};

#endif // DIHEDRAL_BENDING_CONSTRAINT_H