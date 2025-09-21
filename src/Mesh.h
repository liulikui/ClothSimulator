#ifndef MESH_H
#define MESH_H

#include "Primitive.h"
#include <DirectXMath.h>
#include <vector>
#include <cstdint>

namespace dx = DirectX;

class Mesh : public Primitive 
{
public:
    // 虚析构函数
    virtual ~Mesh() = default;
    
    // 获取顶点位置数据
    virtual const std::vector<dx::XMFLOAT3>& GetPositions() const = 0;
    
    // 获取顶点法线数据
    virtual const std::vector<dx::XMFLOAT3>& GetNormals() const = 0;
    
    // 获取索引数据
    virtual const std::vector<uint32_t>& GetIndices() const = 0;
    
    // 获取对象的材质颜色
    const dx::XMFLOAT4& GetDiffuseColor() const 
    {
        return diffuseColor;
    }
    
    // 设置对象的材质颜色
    void SetDiffuseColor(const dx::XMFLOAT4& color)
    {
        diffuseColor = color;
    }
};

#endif // MESH_H