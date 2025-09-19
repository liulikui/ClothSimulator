#ifndef SCENE_H
#define SCENE_H

#include "Mesh.h"
#include "TSharePtr.h"
#include "RALResource.h"
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
    
    // 初始化场景，创建根签名
    // 参数：
    //   pRender - DX12Renderer对象指针
    // 返回值：
    //   初始化是否成功
    bool Initialize(DX12Renderer* pRender);
    
    // 析构函数
    ~Scene();
    
    // 获取场景中Mesh对象的数量
    size_t getMeshCount() const {
        return m_primitives.size();
    }

    // 更新场景中所有对象的状态
    void update(float deltaTime);

    // 添加一个Mesh对象到场景中
    // 参数：
    //   mesh - 要添加的Mesh对象指针
    // 返回值：
    //   添加是否成功
    bool addPrimitive(std::shared_ptr<Mesh> mesh);

    // 从场景中移除一个Mesh对象
    // 参数：
    //   mesh - 要移除的Mesh对象指针
    // 返回值：
    //   移除是否成功
    bool removePrimitive(std::shared_ptr<Mesh> mesh);

    // 清空场景中的所有对象
    void clear();

    // 获取场景中的所有Mesh对象
    const std::vector<std::shared_ptr<Mesh>>& getPrimitives() const {
        return m_primitives;
    }

    // 设置场景的背景颜色
    void setBackgroundColor(const dx::XMFLOAT4& color) {
        m_backgroundColor = color;
    }

    // 获取场景的背景颜色
    const dx::XMFLOAT4& getBackgroundColor() const {
        return m_backgroundColor;
    }

    // 设置场景的光源位置
    void setLightPosition(const dx::XMFLOAT4& position) {
        m_lightPosition = position;
    }

    // 获取场景的光源位置
    const dx::XMFLOAT4& getLightPosition() const {
        return m_lightPosition;
    }

    // 设置场景的光源颜色
    void setLightColor(const dx::XMFLOAT4& color) {
        m_lightColor = color;
    }

    // 获取场景的光源颜色
    const dx::XMFLOAT4& getLightColor() const {
        return m_lightColor;
    }
    
    // 渲染场景
    // 参数：
    //   renderer - 用于渲染的DX12Renderer对象
    //   viewMatrix - 视图矩阵
    //   projectionMatrix - 投影矩阵
    void render(DX12Renderer* renderer, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);

private:
    // 场景中的所有Mesh对象
    std::vector<std::shared_ptr<Mesh>> m_primitives;

    // 场景的背景颜色
    dx::XMFLOAT4 m_backgroundColor = {0.9f, 0.9f, 0.9f, 1.0f}; // 默认浅灰色背景

    // 光源属性
    dx::XMFLOAT4 m_lightPosition = {10.0f, 10.0f, 10.0f, 1.0f}; // 默认光源位置
    dx::XMFLOAT4 m_lightColor = {1.0f, 1.0f, 1.0f, 1.0f};       // 默认光源颜色（白色）

    // 根签名对象
    TSharePtr<IRALRootSignature> m_rootSignature;

    // 着色器对象
    TSharePtr<IRALVertexShader> m_vertexShader;
    TSharePtr<IRALPixelShader> m_pixelShader;

    // 图形管道状态对象
    TSharePtr<IRALGraphicsPipelineState> m_pipelineState;
};

#endif // SCENE_H