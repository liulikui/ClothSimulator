#ifndef DIHEDRAL_BENDING_CONSTRAINT_H
#define DIHEDRAL_BENDING_CONSTRAINT_H

#include "Constraint.h"
#include <DirectXMath.h>

#ifdef DEBUG_SOLVER
#include <string>
extern void logDebug(const std::string& message);
#endif

// 命名空间别名简化使用
namespace dx = DirectX;

// 基于二面角的弯曲约束类，继承自约束基类
// 适配DirectX左手坐标系，严格遵循"二面角=180°-法向量夹角"的几何关系
class DihedralBendingConstraint : public Constraint
{
public:
    // 构造函数
    // 参数：
    //   p1, p2 - 两个三角形共享边的顶点（公共边为p1-p2）
    //   p3 - 第一个三角形的第三个顶点（三角形1：p1-p2-p3）
    //   p4 - 第二个三角形的第三个顶点（三角形2：p1-p2-p4）
    //   compliance - 约束的柔度
    DihedralBendingConstraint(Particle* p1, Particle* p2, Particle* p3, Particle* p4, 
        float compliance, float damping)
        : Constraint(compliance, damping)
        , m_particle1(p1)
        , m_particle2(p2)
        , m_particle3(p3)
        , m_particle4(p4)
    {
        m_restDihedralAngle = GetDihedralAngle(p1, p2, p3, p4);
    }

    // 计算约束偏差
    // 返回：约束偏差值C = 当前二面角 - 静止二面角（二面角范围[0, dx::XM_2PI]）
    float ComputeConstraintAndGradient(dx::XMFLOAT3* gradients) const override
    {
        dx::XMVECTOR p1 = dx::XMLoadFloat3(&m_particle1->position);
        dx::XMVECTOR p2 = dx::XMLoadFloat3(&m_particle2->position);
        dx::XMVECTOR p3 = dx::XMLoadFloat3(&m_particle3->position);
        dx::XMVECTOR p4 = dx::XMLoadFloat3(&m_particle4->position);

        dx::XMVECTOR e2 = dx::XMVectorSubtract(p2, p1);
        dx::XMVECTOR e3 = dx::XMVectorSubtract(p3, p1);
        dx::XMVECTOR e4 = dx::XMVectorSubtract(p4, p1);

        dx::XMVECTOR e2_x_e3 = dx::XMVector3Cross(e2, e3);
        float len_e2_x_e3 = dx::XMVectorGetX(dx::XMVector3Length(e2_x_e3));

        if (len_e2_x_e3 < 1e-6f)
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[2] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            gradients[3] = dx::XMFLOAT3(0.0f, -1.0f, 0.0f);
            return 0.0f;
        }

        dx::XMVECTOR e2_x_e4 = dx::XMVector3Cross(e2, e4);
        float len_e2_x_e4 = dx::XMVectorGetX(dx::XMVector3Length(e2_x_e4));

        if (len_e2_x_e4 < 1e-6f)
        {
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[2] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            gradients[3] = dx::XMFLOAT3(0.0f, -1.0f, 0.0f);
            return 0.0f;
        }

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
        float d = dx::XMVectorGetX(dx::XMVector3Dot(n1Norm, n2Norm));

        // 钳位避免acos输入越界（浮点误差导致）
        d = dx::XMVectorClamp(dx::XMVectorReplicate(d),
            dx::XMVectorReplicate(-1.0f),
            dx::XMVectorReplicate(1.0f)).m128_f32[0];

        // 计算法向量夹角（范围[0, M_PI]）
        float normalAngle = acosf(d);

        dx::XMVECTOR crossNN = dx::XMVector3Normalize(dx::XMVector3Cross(n1Norm, n2Norm));
        float dir = dx::XMVectorGetX(dx::XMVector3Dot(crossNN, e2));

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
        sprintf_s(buffer, "[DEBUG] p1:%f,%f,%f, p2:%f,%f,%f, p3:%f,%f,%f, p4:%f,%f,%f, d:%f dir:%f, normalAngle:%f currentDihedralAngle:%f"
            , m_particle1->position.x
            , m_particle1->position.y
            , m_particle1->position.z
            , m_particle2->position.x
            , m_particle2->position.y
            , m_particle2->position.z
            , m_particle3->position.x
            , m_particle3->position.y
            , m_particle3->position.z
            , m_particle4->position.x
            , m_particle4->position.y
            , m_particle4->position.z
            , d, dir, normalAngle, currentDihedralAngle);
        logDebug(buffer);
#endif//DEBUG_SOLVER

		if (fabs(d - 1.0f) < 1e-6f) // d约等于1，法向量平行且方向相同
        {
            // 共面且方向相同，梯度为零
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[2] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            gradients[3] = dx::XMFLOAT3(0.0f, -1.0f, 0.0f);

			return currentDihedralAngle - m_restDihedralAngle;
        }
		else if (fabs(d + 1.0f) < 1e-6f) // d约等于-1，法向量平行但方向相反
        {
            // 共面但方向相反，梯度无法定义，使用任意垂直于公共边的方向
            dx::XMVECTOR e2Norm = dx::XMVector3Normalize(e2);
            dx::XMVECTOR arbitrary = dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
            if (fabs(dx::XMVectorGetX(dx::XMVector3Dot(e2Norm, arbitrary))) > 0.99f)
            {
                arbitrary = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            }
            dx::XMVECTOR perp = dx::XMVector3Normalize(dx::XMVector3Cross(e2Norm, arbitrary));
            dx::XMVECTOR q1 = dx::XMVectorScale(perp, -1.0f);
            dx::XMVECTOR q2 = dx::XMVectorZero();
            dx::XMVECTOR q3 = dx::XMVectorScale(perp, 1.0f);
            dx::XMVECTOR q4 = dx::XMVectorScale(perp, -1.0f);
            dx::XMStoreFloat3(&gradients[0], q1);
            dx::XMStoreFloat3(&gradients[1], q2);
            dx::XMStoreFloat3(&gradients[2], q3);
            dx::XMStoreFloat3(&gradients[3], q4);

            return currentDihedralAngle - m_restDihedralAngle;
		}
        else
        {
            dx::XMVECTOR e2_x_n2 = dx::XMVector3Cross(e3, n2Norm);
            dx::XMVECTOR n1_x_e2 = dx::XMVector3Cross(n1Norm, e2);
            
            dx::XMVECTOR e2_x_n1 = dx::XMVector3Cross(e2, n1Norm);
            dx::XMVECTOR n2_x_e2 = dx::XMVector3Cross(n2Norm, e2);
            
            dx::XMVECTOR e3_x_n2 = dx::XMVector3Cross(e3, n2Norm);
            dx::XMVECTOR n1_x_e3 = dx::XMVector3Cross(n1Norm, e3);
            dx::XMVECTOR e4_x_n1 = dx::XMVector3Cross(e4, n1Norm);
            dx::XMVECTOR n2_x_e4 = dx::XMVector3Cross(n2Norm, e4);

            dx::XMVECTOR q3 = dx::XMVectorScale((dx::XMVectorAdd(e2_x_n2, dx::XMVectorScale(n1_x_e2, d))), 1 / len_e2_x_e3);
            dx::XMVECTOR q4 = dx::XMVectorScale((dx::XMVectorAdd(e2_x_n1, dx::XMVectorScale(n2_x_e2, d))), 1 / len_e2_x_e4);
            dx::XMVECTOR q2 = dx::XMVectorSubtract(dx::XMVectorScale((dx::XMVectorAdd(e3_x_n2, dx::XMVectorScale(n1_x_e3, d))), 1 / len_e2_x_e3),
                dx::XMVectorScale((dx::XMVectorAdd(e4_x_n1, dx::XMVectorScale(n2_x_e4, d))), 1 / len_e2_x_e4));

            dx::XMVECTOR q1 = dx::XMVectorScale(dx::XMVectorAdd(dx::XMVectorAdd(q2, q3), q4), -1.0f);

            float inv_one_minus_d_squared = 1.0f / sqrt(1.0f - d * d);

            dx::XMStoreFloat3(&gradients[0], dx::XMVectorScale(q1, inv_one_minus_d_squared));
            dx::XMStoreFloat3(&gradients[1], dx::XMVectorScale(q2, inv_one_minus_d_squared));
            dx::XMStoreFloat3(&gradients[2], dx::XMVectorScale(q3, inv_one_minus_d_squared));
            dx::XMStoreFloat3(&gradients[3], dx::XMVectorScale(q4, inv_one_minus_d_squared));

            return currentDihedralAngle - m_restDihedralAngle;
        }
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

    virtual void Check() const override
    {
#ifdef DEBUG_SOLVER
        float currentDihedralAngle = GetDihedralAngle(m_particle1, m_particle2, m_particle3, m_particle4);

        char buffer[256];
        sprintf_s(buffer, "[DEBUG] alfter apply constraint currentDihedralAngle:%f", currentDihedralAngle);
        logDebug(buffer);
#endif
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

    float GetDihedralAngle(Particle* a, Particle* b, Particle* c, Particle* d) const
    {
        dx::XMVECTOR n1 = ComputeTriangleNormal(a, b, c);
        dx::XMVECTOR n2 = ComputeTriangleNormal(a, b, d);
        // 归一化法向量
        dx::XMVECTOR n1Norm = dx::XMVector3Normalize(n1);
        dx::XMVECTOR n2Norm = dx::XMVector3Normalize(n2);
        // 计算法向量点积（法向量夹角的余弦值）
        float dDot = dx::XMVectorGetX(dx::XMVector3Dot(n1Norm, n2Norm));
        // 钳位避免acos输入越界（浮点误差导致）
        dDot = dx::XMVectorClamp(dx::XMVectorReplicate(dDot),
            dx::XMVectorReplicate(-1.0f),
            dx::XMVectorReplicate(1.0f)).m128_f32[0];
        // 计算法向量夹角（范围[0, M_PI]）
        float normalAngle = acosf(dDot);
        // 公共边向量（p2 - p1）
        dx::XMVECTOR edge = dx::XMVectorSubtract(dx::XMLoadFloat3(&b->position),
            dx::XMLoadFloat3(&a->position));
        dx::XMVECTOR crossNN = dx::XMVector3Normalize(dx::XMVector3Cross(n1Norm, n2Norm));
        float dir = dx::XMVectorGetX(dx::XMVector3Dot(crossNN, edge));

        if (dir > 0.0f)
        {
            return normalAngle;
        }
        else
        {
            return dx::XM_2PI - normalAngle;
        }
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