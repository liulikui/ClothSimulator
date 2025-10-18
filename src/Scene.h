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
    
    // 设置场景的光源方向（自动归一化）
    void SetLightDirection(const dx::XMFLOAT3& direction);

    // 获取场景的光源方向
    const dx::XMFLOAT3& GetLightDirection() const 
    { 
        return m_lightDirection; 
    }
    
    // 设置场景的环境光颜色
    void SetLightAmbientColor(const dx::XMFLOAT4& color)
    { 
        m_lightAmbientColor = color; 
    }
    
    // 获取场景的环境光颜色
    const dx::XMFLOAT4& GetLightAmbientColor() const 
    { 
        return m_lightAmbientColor; 
    }
    
    // 调整渲染资源大小
    void Resize(uint32_t width, uint32_t height);

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

    // 初始化延迟着色相关资源
    bool InitializeDeferredRendering();
    // 清理延迟着色相关资源
    void CleanupDeferredRendering();
    // 创建渲染目标、深度模板和相关视图资源
    bool CreateRenderingResources(uint32_t width, uint32_t height);
    // 释放所有渲染资源
    void ReleaseRenderingResources();
    
    // 创建全屏四边形
    void CreateFullscreenQuad();

    // 执行几何阶段
    void ExecuteGeometryPass(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);
    // 执行光照阶段
    void ExecuteLightingPass();
    // 执行GBuffer Resolve阶段
    void ExecuteResolvePass();
	// 执行色调映射阶段
    void ExecuteTonemappingPass();

private:
    IRALDevice* m_device;

    // 添加Primitive的请求列表
    std::vector<AddPrimitiveRequest> m_addPrimitiveRequests;

    // 场景中的所有Mesh对象
    std::vector<PrimitiveInfo> m_primitives;

    // 场景的背景颜色
    dx::XMFLOAT4 m_backgroundColor; // 默认浅灰色背景

    // 光源属性
    dx::XMFLOAT3 m_lightPosition; // 默认光源位置
    dx::XMFLOAT3 m_lightDirection; // 默认光源方向
    dx::XMFLOAT4 m_lightDiffuseColor;       // 默认光源颜色（白色）
    dx::XMFLOAT4 m_lightSpecularColor;       // 默认光源颜色（白色）
    dx::XMFLOAT4 m_lightAmbientColor;        // 默认环境光颜色

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
    
    // 延迟着色相关 - Resolve阶段管道状态
    TRefCountPtr<IRALGraphicsPipelineState> m_resolvePipelineState;
    TRefCountPtr<IRALVertexShader> m_resolveVertexShader;
    TRefCountPtr<IRALPixelShader> m_resolvePixelShader;
    TRefCountPtr<IRALRootSignature> m_resolveRootSignature;

    // GBuffer相关
    TRefCountPtr<IRALRenderTarget> m_gbufferA; // RRG为世界空间法线
    TRefCountPtr<IRALRenderTarget> m_gbufferB; // Metallic, Specular, Roughness
    TRefCountPtr<IRALRenderTarget> m_gbufferC; // BaseColor RGB
    TRefCountPtr<IRALDepthStencil> m_gbufferDepthStencil;
    
    // GBuffer对应的视图
    TRefCountPtr<IRALRenderTargetView> m_gbufferARTV;
    TRefCountPtr<IRALRenderTargetView> m_gbufferBRTV;
    TRefCountPtr<IRALRenderTargetView> m_gbufferCRTV;
    TRefCountPtr<IRALDepthStencilView> m_gbufferDSV;
    
    // GBuffer对应的SRV（用于光照阶段采样）
    TRefCountPtr<IRALShaderResourceView> m_gbufferASRV;
    TRefCountPtr<IRALShaderResourceView> m_gbufferBSRV;
    TRefCountPtr<IRALShaderResourceView> m_gbufferCSRV;
    TRefCountPtr<IRALShaderResourceView> m_gbufferDepthSRV;
    
    // 光照结果RT（用于保存Diffuse和Specular的光照计算结果）
    TRefCountPtr<IRALRenderTarget> m_diffuseLightRT; // 保存Diffuse光照计算结果
    TRefCountPtr<IRALRenderTarget> m_specularLightRT; // 保存Specular光照计算结果
    
    // 光照结果RT对应的视图
    TRefCountPtr<IRALRenderTargetView> m_diffuseLightRTV;
    TRefCountPtr<IRALRenderTargetView> m_specularLightRTV;
    TRefCountPtr<IRALShaderResourceView> m_diffuseLightSRV;
    TRefCountPtr<IRALShaderResourceView> m_specularLightSRV;
    
    // HDR场景颜色渲染目标（用于延迟着色Resolve结果）
    TRefCountPtr<IRALRenderTarget> m_HDRSceneColor;        // HDR场景颜色RT
    TRefCountPtr<IRALRenderTargetView> m_HDRSceneColorRTV; // HDR场景颜色RTV
    TRefCountPtr<IRALShaderResourceView> m_HDRSceneColorSRV; // HDR场景颜色SRV
    
    // 色调映射相关
    TRefCountPtr<IRALRootSignature> m_tonemappingRootSignature;          // 色调映射根签名
    TRefCountPtr<IRALGraphicsPipelineState> m_tonemappingPipelineState;   // 色调映射管线状态
    TRefCountPtr<IRALVertexShader> m_tonemappingVS;                       // 色调映射顶点着色器
    TRefCountPtr<IRALPixelShader> m_tonemappingPS;                        // 色调映射像素着色器
    
    TRefCountPtr<IRALConstBuffer> m_lightPassConstBuffer;
    TRefCountPtr<IRALVertexBuffer> m_fullscreenQuadVB;
    TRefCountPtr<IRALIndexBuffer> m_fullscreenQuadIB;

    // 场景相关常量
    TRefCountPtr<IRALConstBuffer> m_sceneConstBuffer;
};

#endif // SCENE_H