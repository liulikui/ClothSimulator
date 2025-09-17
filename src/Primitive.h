#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <DirectXMath.h>
#include <vector>
#include <cstdint>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 前向声明
class DX12Renderer;

class Primitive {
public:
    // 虚析构函数，确保派生类能够正确析构
    virtual ~Primitive() = default;

    // 纯虚函数，更新对象状态
    virtual void update(float deltaTime) = 0;

    // 获取顶点位置数据
    virtual const std::vector<dx::XMFLOAT3>& getPositions() const = 0;

    // 获取顶点法线数据
    virtual const std::vector<dx::XMFLOAT3>& getNormals() const = 0;

    // 获取索引数据
    virtual const std::vector<uint32_t>& getIndices() const = 0;

    // 获取对象的世界变换矩阵
    const dx::XMMATRIX& getWorldMatrix() const {
        return worldMatrix;
    }

    // 设置对象的世界变换矩阵
    void setWorldMatrix(const dx::XMMATRIX& matrix) {
        worldMatrix = matrix;
    }

    // 设置对象的位置
    void setPosition(const dx::XMFLOAT3& position);

    // 设置对象的旋转（欧拉角，弧度）
    void setRotation(const dx::XMFLOAT3& rotation);

    // 设置对象的缩放
    void setScale(const dx::XMFLOAT3& scale);

    // 获取对象的材质颜色
    const dx::XMFLOAT4& getDiffuseColor() const {
        return diffuseColor;
    }

    // 设置对象的材质颜色
    void setDiffuseColor(const dx::XMFLOAT4& color) {
        diffuseColor = color;
    }

    // 是否可见
    bool isVisible() const {
        return visible;
    }

    // 设置可见性
    void setVisible(bool isVisible) {
        visible = isVisible;
    }

protected:
    // 世界变换矩阵
    dx::XMMATRIX worldMatrix = dx::XMMatrixIdentity();

    // 对象的位置、旋转和缩放
    dx::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    dx::XMFLOAT3 rotation = {0.0f, 0.0f, 0.0f};
    dx::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};

    // 材质颜色
    dx::XMFLOAT4 diffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};

    // 是否可见
    bool visible = true;

    // 更新世界矩阵
    void updateWorldMatrix();
};

#endif // PRIMITIVE_H