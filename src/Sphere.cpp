#include "Sphere.h"
#include <DirectXMath.h>
#include <cmath>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

Sphere::Sphere(const dx::XMFLOAT3& center, float radius, uint32_t sectors, uint32_t stacks)
    : center(center), radius(radius), sectors(sectors), stacks(stacks) {
    // 设置球体的位置
    setPosition(center);
    
    // 生成球体数据
    generateSphereData();
}

void Sphere::update(float deltaTime) {
    // 球体默认不需要复杂的更新逻辑
    // 可以在这里添加动画或其他动态行为
}

void Sphere::setRadius(float newRadius) {
    if (radius != newRadius) {
        radius = newRadius;
        generateSphereData();
    }
}

void Sphere::setCenter(const dx::XMFLOAT3& newCenter) {
    if (center.x != newCenter.x || center.y != newCenter.y || center.z != newCenter.z) {
        center = newCenter;
        setPosition(center);
    }
}

void Sphere::generateSphereData() {
    // 清空现有数据
    positions.clear();
    normals.clear();
    indices.clear();

    // 生成顶点和法线
    for (uint32_t i = 0; i <= stacks; ++i) {
        float phi = dx::XMConvertToRadians(180.0f * static_cast<float>(i) / stacks); // 极角
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (uint32_t j = 0; j <= sectors; ++j) {
            float theta = dx::XMConvertToRadians(360.0f * static_cast<float>(j) / sectors); // 方位角
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // 添加顶点位置
            dx::XMFLOAT3 pos;
            pos.x = radius * sinPhi * cosTheta;
            pos.y = radius * cosPhi;
            pos.z = radius * sinPhi * sinTheta;
            positions.push_back(pos);

            // 添加法线（单位向量）
            dx::XMFLOAT3 normal;
            normal.x = sinPhi * cosTheta;
            normal.y = cosPhi;
            normal.z = sinPhi * sinTheta;
            normals.push_back(normal);
        }
    }

    // 生成索引
    for (uint32_t i = 0; i < stacks; ++i) {
        uint32_t row1 = i * (sectors + 1);
        uint32_t row2 = (i + 1) * (sectors + 1);

        for (uint32_t j = 0; j < sectors; ++j) {
            // 第一个三角形
            indices.push_back(row1 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row1 + j + 1);

            // 第二个三角形
            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
        }
    }
}