#ifndef SCENE_H
#define SCENE_H

#include "Primitive.h"
#include "TRefCountPtr.h"
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
    
    // 获取场景中Primitive对象的数量
    size_t GetPrimitiveCount() const
    {
        return m_primitives.size();
    }

    // 更新场景中所有Primitive对象的状态
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
        TRefCountPtr<IRALVertexBuffer> vertexBuffer;
        TRefCountPtr<IRALIndexBuffer> indexBuffer;
        TRefCountPtr<IRALConstBuffer> constBuffer;
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
    TRefCountPtr<IRALRootSignature> m_rootSignature;

    // 着色器对象
    TRefCountPtr<IRALVertexShader> m_vertexShader;
    TRefCountPtr<IRALPixelShader> m_pixelShader;

    // 图形管道状态对象
    TRefCountPtr<IRALGraphicsPipelineState> m_pipelineState;

    // 延迟着色相关 - 几何阶段管道状态
    TRefCountPtr<IRALGraphicsPipelineState> m_gbufferPipelineState;
    TRefCountPtr<IRALVertexShader> m_gbufferVertexShader;
    TRefCountPtr<IRALPixelShader> m_gbufferPixelShader;
    TRefCountPtr<IRALRootSignature> m_gbufferRootSignature;

    // 延迟着色相关 - 光照阶段管道状态
    TRefCountPtr<IRALGraphicsPipelineState> m_lightPipelineState;
    TRefCountPtr<IRALVertexShader> m_lightVertexShader;
    TRefCountPtr<IRALPixelShader> m_lightPixelShader;
    TRefCountPtr<IRALRootSignature> m_lightRootSignature;

    // GBuffer相关
    TRefCountPtr<IRALRenderTarget> m_gbufferA; // 世界空间法线
    TRefCountPtr<IRALRenderTarget> m_gbufferB; // Metallic, Specular, Roughness
    TRefCountPtr<IRALRenderTarget> m_gbufferC; // BaseColor RGB
    TRefCountPtr<IRALDepthStencil> m_gbufferDepthStencil;
    TRefCountPtr<IRALConstBuffer> m_lightPassConstBuffer;
    TRefCountPtr<IRALVertexBuffer> m_fullscreenQuadVB;
    TRefCountPtr<IRALIndexBuffer> m_fullscreenQuadIB;

    // 场景相关常量
    TRefCountPtr<IRALConstBuffer> m_constBuffer;

    // 初始化延迟着色相关资源
    bool InitializeDeferredRendering();
    // 清理延迟着色相关资源
    void CleanupDeferredRendering();
    // 执行几何阶段
    void ExecuteGeometryPass(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);
    // 执行光照阶段
    void ExecuteLightingPass();
    // 创建全屏四边形
    void CreateFullscreenQuad();
};

#endif // SCENE_H