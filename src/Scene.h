#ifndef SCENE_H
#define SCENE_H

#include "Primitive.h"
#include "TSharePtr.h"
#include "RALResource.h"
#include <vector>
#include <memory>
#include <DirectXMath.h>

// 前向声明
class IRALDevice;

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

class Scene
{
public:
    // 构造函数
    Scene();
    
    // 初始化场景，创建根签名
    // 参数：
    //   pDevice - IRALDevice对象指针
    // 返回值：
    //   初始化是否成功
    bool Initialize(IRALDevice* pDevice);
    
    // 析构函数
    ~Scene();
    
    // 获取场景中Mesh对象的数量
    size_t GetMeshCount() const {
        return m_primitives.size();
    }

    // 更新场景中所有对象的状态
    void Update(float deltaTime);

    // 渲染场景
   // 参数：
   //   commandList - 命令队列
   //   viewMatrix - 视图矩阵
   //   projectionMatrix - 投影矩阵
    void Render(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);

    // 添加一个Primitive对象到场景中
    // 参数：
    //   primitive - 要添加的Primitive对象指针
    // 返回值：
    //   添加是否成功
    bool AddPrimitive(Primitive* primitive);

    // 从场景中移除一个Primitive对象
    // 参数：
    //   primitive - 要移除的primitive对象指针
    // 返回值：
    //   移除是否成功
    bool RemovePrimitive(Primitive* primitive);

    // 清空场景中的所有对象
    void Clear();

    // 设置场景的背景颜色
    void SetBackgroundColor(const dx::XMFLOAT4& color)
    {
        m_backgroundColor = color;
    }

    // 获取场景的背景颜色
    const dx::XMFLOAT4& GetBackgroundColor() const
    {
        return m_backgroundColor;
    }

    // 设置场景的光源位置
    void SetLightPosition(const dx::XMFLOAT3& position)
    {
        m_lightPosition = position;
    }

    // 获取场景的光源位置
    const dx::XMFLOAT3& GetLightPosition() const 
    {
        return m_lightPosition;
    }

    // 设置场景的光源颜色
    void SetLightDiffuseColor(const dx::XMFLOAT4& color)
    {
        m_lightDiffuseColor = color;
    }

    // 获取场景的光源颜色
    const dx::XMFLOAT4& GetLightDiffuseColor() const 
    {
        return m_lightDiffuseColor;
    }

private:
    struct AddPrimitiveRequest
    {
        Primitive* primitive;
    };

    struct PrimitiveInfo
    {
        Primitive* primitive;
        dx::XMMATRIX worldMatrix;
        bool visible;
        dx::XMFLOAT3 diffuseColor;
        TSharePtr<IRALVertexBuffer> vertexBuffer;
        TSharePtr<IRALIndexBuffer> indexBuffer;
        TSharePtr<IRALConstBuffer> constBuffer;
    };

    void UpdatePrimitiveRequests();
    void UpdateSceneConstBuffer(IRALGraphicsCommandList* commandList, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);
	void UpdatePrimitiveConstBuffer(IRALGraphicsCommandList* commandList, PrimitiveInfo* primitiveInfo);

private:
    IRALDevice* m_device;

    // 添加Primitive的请求列表
    std::vector<AddPrimitiveRequest> m_addPrimitiveRequests;

    // 场景中的所有Mesh对象
    std::vector<PrimitiveInfo> m_primitives;

    // 场景的背景颜色
    dx::XMFLOAT4 m_backgroundColor = {0.9f, 0.9f, 0.9f, 1.0f}; // 默认浅灰色背景

    // 光源属性
    dx::XMFLOAT3 m_lightPosition = {10.0f, 10.0f, 10.0f}; // 默认光源位置
    dx::XMFLOAT4 m_lightDiffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};       // 默认光源颜色（白色）
    dx::XMFLOAT4 m_lightSpecularColor = { 1.0f, 1.0f, 1.0f, 1.0f };       // 默认光源颜色（白色）

    // 根签名对象
    TSharePtr<IRALRootSignature> m_rootSignature;

    // 着色器对象
    TSharePtr<IRALVertexShader> m_vertexShader;
    TSharePtr<IRALPixelShader> m_pixelShader;

    // 图形管道状态对象
    TSharePtr<IRALGraphicsPipelineState> m_pipelineState;

    // 场景相关常量
    TSharePtr<IRALConstBuffer> m_constBuffer;
};

#endif // SCENE_H