#ifndef DIHEDRAL_BENDING_CONSTRAINT_H
#define DIHEDRAL_BENDING_CONSTRAINT_H

#include "Constraint.h"
#include <DirectXMath.h>

// 命名空间别名简化使用
namespace dx = DirectX;

// 基于二面角的弯曲约束类，继承自约束基类
// 该约束保持两个相邻三角形之间的二面角为指定值（更真实地模拟表面弯曲）
class DihedralBendingConstraint : public Constraint 
{
public:
    // 构造函数
    // 参数：
    //   p0, p1 - 两个三角形共享边的顶点（公共边为p0-p1）
    //   p2 - 第一个三角形的第三个顶点（三角形1：p0-p1-p2）
    //   p3 - 第二个三角形的第三个顶点（三角形2：p0-p1-p3）
    //   restDihedralAngle - 约束的静止二面角（弧度）
    //   compliance - 约束的柔度（与刚度成反比，值越小刚度越大）
    DihedralBendingConstraint(Particle* p0, Particle* p1, Particle* p2, Particle* p3, 
                             float restDihedralAngle, float compliance = 1e-6)
        : particle0(p0), particle1(p1), particle2(p2), particle3(p3), 
          restDihedralAngle(restDihedralAngle)
    {
        this->compliance = compliance;
        // 预计算静止二面角的正弦和余弦（用于梯度计算优化）
        sinRestAngle = sinf(restDihedralAngle);
        cosRestAngle = cosf(restDihedralAngle);
    }
    
    // 计算约束偏差
    // 返回：约束偏差值C = 当前二面角 - 静止二面角
    float ComputeConstraintValue() const override 
    {
        // 计算两个三角形的法向量（未归一化）
        dx::XMVECTOR n1 = ComputeTriangleNormal(particle0, particle1, particle2);
        dx::XMVECTOR n2 = ComputeTriangleNormal(particle0, particle1, particle3);
        
        // 法向量长度（用于归一化）
        float len1 = dx::XMVectorGetX(dx::XMVector3Length(n1));
        float len2 = dx::XMVectorGetX(dx::XMVector3Length(n2));
        
        // 避免法向量为零（三角形退化）
        if (len1 < 1e-9f || len2 < 1e-9f)
            return 0.0f;
        
        // 归一化法向量
        dx::XMVECTOR n1Norm = dx::XMVectorDivide(n1, dx::XMVectorReplicate(len1));
        dx::XMVECTOR n2Norm = dx::XMVectorDivide(n2, dx::XMVectorReplicate(len2));
        
        // 计算法向量点积（二面角的余弦值）
        float dotNN = dx::XMVectorGetX(dx::XMVector3Dot(n1Norm, n2Norm));
        dotNN = dx::XMVectorClamp(dx::XMVectorReplicate(dotNN), 
                                 dx::XMVectorReplicate(-1.0f), 
                                 dx::XMVectorReplicate(1.0f)).m128_f32[0];
        
        // 计算二面角方向（通过公共边向量与法向量叉积判断）
        dx::XMVECTOR edge = dx::XMVectorSubtract(dx::XMLoadFloat3(&particle1->position), dx::XMLoadFloat3(&particle0->position));
        dx::XMVECTOR crossNN = dx::XMVector3Cross(n1Norm, n2Norm);
        float dir = dx::XMVectorGetX(dx::XMVector3Dot(crossNN, edge));
        float sign = (dir > 0) ? 1.0f : -1.0f;
        
        // 当前二面角 = 符号 * 反余弦（点积）
        float currentAngle = sign * acosf(dotNN);
        
        // 约束偏差：当前二面角 - 静止二面角
        return currentAngle - restDihedralAngle;
    }
    
    // 计算约束梯度
    // 参数：
    //   gradients - 存储每个受约束粒子的梯度向量的数组（大小为4）
    void ComputeGradient(dx::XMFLOAT3* gradients) const override
    {
        // 计算两个三角形的法向量（未归一化）及长度
        dx::XMVECTOR n1 = ComputeTriangleNormal(particle0, particle1, particle2);
        dx::XMVECTOR n2 = ComputeTriangleNormal(particle0, particle1, particle3);
        float len1 = dx::XMVectorGetX(dx::XMVector3Length(n1));
        float len2 = dx::XMVectorGetX(dx::XMVector3Length(n2));
        
        // 处理退化三角形（法向量为零）
        if (len1 < 1e-9f || len2 < 1e-9f)
        {
            // 退化时使用默认梯度（避免除零）
            gradients[0] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[1] = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
            gradients[2] = dx::XMFLOAT3(0.0f, 1.0f, 0.0f);
            gradients[3] = dx::XMFLOAT3(0.0f, -1.0f, 0.0f);
            return;
        }
        
        // 归一化法向量
        dx::XMVECTOR n1Norm = dx::XMVectorDivide(n1, dx::XMVectorReplicate(len1));
        dx::XMVECTOR n2Norm = dx::XMVectorDivide(n2, dx::XMVectorReplicate(len2));
        
        // 公共边向量（p1 - p0）
        dx::XMVECTOR e = dx::XMVectorSubtract(dx::XMLoadFloat3(&particle1->position), dx::XMLoadFloat3(&particle0->position));
        dx::XMVECTOR eNorm = dx::XMVector3Normalize(e);
        
        // 计算二面角的正弦值（用于梯度归一化）
        dx::XMVECTOR crossNN = dx::XMVector3Cross(n1Norm, n2Norm);
        float sinTheta = dx::XMVectorGetX(dx::XMVector3Length(crossNN));
        sinTheta = (sinTheta < 1e-9f) ? 1e-9f : sinTheta; // 避免除零
        
        // 计算每个顶点对两个法向量的梯度贡献
        dx::XMVECTOR dn1_dp0, dn1_dp1, dn1_dp2;
        ComputeNormalGradients(particle0, particle1, particle2, n1, len1, 
                              dn1_dp0, dn1_dp1, dn1_dp2);
        
        dx::XMVECTOR dn2_dp0, dn2_dp1, dn2_dp3;
        ComputeNormalGradients(particle0, particle1, particle3, n2, len2, 
                              dn2_dp0, dn2_dp1, dn2_dp3);
        
        // 二面角对法向量的导数（链式法则中间量）
        dx::XMVECTOR dTheta_dn1 = dx::XMVectorMultiply(
            dx::XMVector3Cross(n2Norm, eNorm),
            dx::XMVectorReplicate(1.0f / sinTheta)
        );
        
        dx::XMVECTOR dTheta_dn2 = dx::XMVectorMultiply(
            dx::XMVector3Cross(eNorm, n1Norm),
            dx::XMVectorReplicate(1.0f / sinTheta)
        );
        
        // 最终梯度 = 二面角对顶点位置的导数（链式法则）
        gradients[0] = XMVectorToFloat3(dx::XMVectorAdd(
            dx::XMVector3Dot(dn1_dp0, dTheta_dn1),
            dx::XMVector3Dot(dn2_dp0, dTheta_dn2)
        ));
        
        gradients[1] = XMVectorToFloat3(dx::XMVectorAdd(
            dx::XMVector3Dot(dn1_dp1, dTheta_dn1),
            dx::XMVector3Dot(dn2_dp1, dTheta_dn2)
        ));
        
        gradients[2] = XMVectorToFloat3(dx::XMVector3Dot(dn1_dp2, dTheta_dn1));
        gradients[3] = XMVectorToFloat3(dx::XMVector3Dot(dn2_dp3, dTheta_dn2));
    }
    
    // 获取受此约束影响的粒子数量
    uint32_t GetParticlesCount() const override
    {
        return 4; // 二面角约束涉及4个顶点
    }

    // 获取受此约束影响的粒子数组
    Particle** GetParticles() override
    {
        return &particle0;
    }

    // 设置约束的静止二面角
    void SetRestDihedralAngle(float angle)
    {
        restDihedralAngle = angle;
        sinRestAngle = sinf(angle);
        cosRestAngle = cosf(angle);
    }
    
    // 获取约束的静止二面角
    float GetRestDihedralAngle() const
    {
        return restDihedralAngle;
    }
    
    // 获取约束类型
    const char* GetConstraintType() const override
    {
        return "DihedralBending";
    }

private:
    // 辅助函数：计算三角形法向量（未归一化）
    dx::XMVECTOR ComputeTriangleNormal(Particle* a, Particle* b, Particle* c) const
    {
        dx::XMVECTOR posA = dx::XMLoadFloat3(&a->position);
        dx::XMVECTOR posB = dx::XMLoadFloat3(&b->position);
        dx::XMVECTOR posC = dx::XMLoadFloat3(&c->position);
        
        dx::XMVECTOR v1 = dx::XMVectorSubtract(posB, posA);
        dx::XMVECTOR v2 = dx::XMVectorSubtract(posC, posA);
        
        return dx::XMVector3Cross(v1, v2); // 法向量 = 边向量叉积
    }
    
    // 辅助函数：计算法向量对三个顶点的梯度（用于链式法则）
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
        
        // 未归一化法向量对顶点的导数（叉积的导数性质）
        dx::XMVECTOR dn_da_unnorm = dx::XMVector3Cross(vAC, vAB); // 对顶点a的导数
        dx::XMVECTOR dn_db_unnorm = dx::XMVector3Cross(vAB, vBC); // 对顶点b的导数
        dx::XMVECTOR dn_dc_unnorm = dx::XMVector3Cross(vBC, vAC); // 对顶点c的导数
        
        // 归一化法向量的导数 = (未归一化导数 - 法向量 * 点积) / 法向量长度
        dx::XMVECTOR normalNorm = dx::XMVectorDivide(normal, dx::XMVectorReplicate(normalLen));
        
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
    Particle* particle0;
    Particle* particle1;
    Particle* particle2;
    Particle* particle3;
    
    // 约束参数
    float restDihedralAngle;  // 静止二面角（弧度）
    float sinRestAngle;       // 静止二面角的正弦值（缓存）
    float cosRestAngle;       // 静止二面角的余弦值（缓存）
};

#endif // DIHEDRAL_BENDING_CONSTRAINT_H
