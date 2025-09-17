#ifndef SPHERE_H
#define SPHERE_H

#include "Primitive.h"
#include <DirectXMath.h>
#include <vector>
#include <cstdint>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

class Sphere : public Primitive {
public:
    // 构造函数：创建一个球体对象
    // 参数：
    //   center - 球体中心位置
    //   radius - 球体半径
    //   sectors - 球体的经度分段数
    //   stacks - 球体的纬度分段数
    Sphere(const dx::XMFLOAT3& center = dx::XMFLOAT3(0.0f, 0.0f, 0.0f),
           float radius = 1.0f,
           uint32_t sectors = 32,
           uint32_t stacks = 32);

    // 析构函数
    ~Sphere() override = default;

    // 更新球体状态
    void update(float deltaTime) override;

    // 获取球体的顶点位置数据
    const std::vector<dx::XMFLOAT3>& getPositions() const override {
        return positions;
    }

    // 获取球体的顶点法线数据
    const std::vector<dx::XMFLOAT3>& getNormals() const override {
        return normals;
    }

    // 获取球体的索引数据
    const std::vector<uint32_t>& getIndices() const override {
        return indices;
    }

    // 获取球体半径
    float getRadius() const {
        return radius;
    }

    // 设置球体半径
    void setRadius(float newRadius);

    // 获取球体中心位置
    const dx::XMFLOAT3& getCenter() const {
        return center;
    }

    // 设置球体中心位置
    void setCenter(const dx::XMFLOAT3& newCenter);

    // 获取经度分段数
    uint32_t getSectors() const {
        return sectors;
    }

    // 获取纬度分段数
    uint32_t getStacks() const {
        return stacks;
    }

private:
    // 球体参数
    dx::XMFLOAT3 center; // 球体中心位置
    float radius;        // 球体半径
    uint32_t sectors;    // 经度分段数
    uint32_t stacks;     // 纬度分段数

    // 渲染数据
    std::vector<dx::XMFLOAT3> positions; // 顶点位置数据
    std::vector<dx::XMFLOAT3> normals;   // 顶点法线数据
    std::vector<uint32_t> indices;       // 索引数据

    // 生成球体的顶点和索引数据
    void generateSphereData();
};

#endif // SPHERE_H