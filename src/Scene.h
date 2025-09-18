#ifndef SCENE_H
#define SCENE_H

#include "Primitive.h"
#include <vector>
#include <memory>
#include <DirectXMath.h>

// 前向声明
class DX12Renderer;

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

class Scene {
public:
    // 构造函数
    Scene();
    
    // 析构函数
    ~Scene();
    
    // 获取场景中Primitive对象的数量
    size_t getPrimitiveCount() const {
        return primitives.size();
    }

    // 更新场景中所有对象的状态
    void update(float deltaTime);

    // 添加一个Primitive对象到场景中
    // 参数：
    //   primitive - 要添加的Primitive对象指针
    // 返回值：
    //   添加是否成功
    bool addPrimitive(std::shared_ptr<Primitive> primitive);

    // 从场景中移除一个Primitive对象
    // 参数：
    //   primitive - 要移除的Primitive对象指针
    // 返回值：
    //   移除是否成功
    bool removePrimitive(std::shared_ptr<Primitive> primitive);

    // 清空场景中的所有对象
    void clear();

    // 获取场景中的所有Primitive对象
    const std::vector<std::shared_ptr<Primitive>>& getPrimitives() const {
        return primitives;
    }

    // 设置场景的背景颜色
    void setBackgroundColor(const dx::XMFLOAT4& color) {
        backgroundColor = color;
    }

    // 获取场景的背景颜色
    const dx::XMFLOAT4& getBackgroundColor() const {
        return backgroundColor;
    }

    // 设置场景的光源位置
    void setLightPosition(const dx::XMFLOAT4& position) {
        lightPosition = position;
    }

    // 获取场景的光源位置
    const dx::XMFLOAT4& getLightPosition() const {
        return lightPosition;
    }

    // 设置场景的光源颜色
    void setLightColor(const dx::XMFLOAT4& color) {
        lightColor = color;
    }

    // 获取场景的光源颜色
    const dx::XMFLOAT4& getLightColor() const {
        return lightColor;
    }
    
    // 渲染场景
    // 参数：
    //   renderer - 用于渲染的DX12Renderer对象
    //   viewMatrix - 视图矩阵
    //   projectionMatrix - 投影矩阵
    void render(DX12Renderer* renderer, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);

private:
    // 场景中的所有Primitive对象
    std::vector<std::shared_ptr<Primitive>> primitives;

    // 场景的背景颜色
    dx::XMFLOAT4 backgroundColor = {0.9f, 0.9f, 0.9f, 1.0f}; // 默认浅灰色背景

    // 光源属性
    dx::XMFLOAT4 lightPosition = {10.0f, 10.0f, 10.0f, 1.0f}; // 默认光源位置
    dx::XMFLOAT4 lightColor = {1.0f, 1.0f, 1.0f, 1.0f};       // 默认光源颜色（白色）
};

#endif // SCENE_H