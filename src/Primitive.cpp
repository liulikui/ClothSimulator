#include "Primitive.h"
#include <DirectXMath.h>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

void Primitive::setPosition(const dx::XMFLOAT3& position) {
    this->position = position;
    updateWorldMatrix();
}

void Primitive::setRotation(const dx::XMFLOAT3& rotation) {
    this->rotation = rotation;
    updateWorldMatrix();
}

void Primitive::setScale(const dx::XMFLOAT3& scale) {
    this->scale = scale;
    updateWorldMatrix();
}

void Primitive::updateWorldMatrix() {
    // 计算旋转矩阵（基于欧拉角）
    dx::XMMATRIX rotX = dx::XMMatrixRotationX(rotation.x);
    dx::XMMATRIX rotY = dx::XMMatrixRotationY(rotation.y);
    dx::XMMATRIX rotZ = dx::XMMatrixRotationZ(rotation.z);

    // 计算缩放矩阵
    dx::XMMATRIX scaleMatrix = dx::XMMatrixScaling(scale.x, scale.y, scale.z);

    // 计算平移矩阵
    dx::XMMATRIX translationMatrix = dx::XMMatrixTranslation(position.x, position.y, position.z);

    // 组合变换矩阵：缩放 -> 旋转 -> 平移
    worldMatrix = scaleMatrix * rotX * rotY * rotZ * translationMatrix;
}