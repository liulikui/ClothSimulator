#ifndef IRAL_DEVICE_H
#define IRAL_DEVICE_H

#include "RALResource.h"
#include "RALCommandList.h"
#include <vector>
#include <string>

// IRALDevice接口定义
class IRALDevice
{
public:
    virtual ~IRALDevice() = default;

    // 初始化设备
    virtual bool Initialize() = 0;

    // 开始一帧
    virtual void BeginFrame() = 0;

    // 结束一帧
    virtual void EndFrame() = 0;

    // 清理资源
    virtual void Cleanup() = 0;

    // 调整窗口大小
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    // 获取窗口宽度
    virtual uint32_t GetWidth() const = 0;

    // 获取窗口高度
    virtual uint32_t GetHeight() const = 0;

    // 编译顶点着色器
    virtual IRALVertexShader* CompileVertexShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译像素着色器
    virtual IRALPixelShader* CompilePixelShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译几何着色器
    virtual IRALGeometryShader* CompileGeometryShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译计算着色器
    virtual IRALComputeShader* CompileComputeShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译网格着色器
    virtual IRALMeshShader* CompileMeshShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译放大着色器
    virtual IRALAmplificationShader* CompileAmplificationShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译光线生成着色器
    virtual IRALRayGenShader* CompileRayGenShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译光线未命中着色器
    virtual IRALRayMissShader* CompileRayMissShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译光线命中组着色器
    virtual IRALRayHitGroupShader* CompileRayHitGroupShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 编译光线可调用着色器
    virtual IRALRayCallableShader* CompileRayCallableShader(const char* shaderCode, const char* entryPoint = "main") = 0;

    // 创建图形管线状态
    virtual IRALGraphicsPipelineState* CreateGraphicsPipelineState(const RALGraphicsPipelineStateDesc& desc) = 0;

    // 创建根签名 
    virtual IRALRootSignature* CreateRootSignature(const std::vector<RALRootParameter>& rootParameters,
        const std::vector<RALStaticSampler>& staticSamplers = {},
        RALRootSignatureFlags flags = RALRootSignatureFlags::AllowInputAssemblerInputLayout) = 0;

    // 创建顶点缓冲区
    virtual IRALVertexBuffer* CreateVertexBuffer(uint32_t size, uint32_t stride, bool isStatic) = 0;

    // 创建索引缓冲区
    virtual IRALIndexBuffer* CreateIndexBuffer(uint32_t count, bool is32BitIndex, bool isStatic) = 0;

    // 创建常量缓冲区
    virtual IRALConstBuffer* CreateConstBuffer(uint32_t size) = 0;

    // 更新Buffer
    virtual bool UploadBuffer(IRALBuffer* buffer, const char* data, uint64_t size) = 0;

    // 获得GraphicsCommandList
    virtual IRALGraphicsCommandList* GetGraphicsCommandList() = 0;

    // 创建渲染目标
    virtual IRALRenderTarget* CreateRenderTarget(uint32_t width, uint32_t height, RALDataFormat format) = 0;

    // 创建渲染目标视图
    virtual IRALRenderTargetView* CreateRenderTargetView(IRALRenderTarget* renderTarget, const RALRenderTargetViewDesc& desc) = 0;

    // 创建深度模板视图
    virtual IRALDepthStencilView* CreateDepthStencilView(IRALDepthStencil* depthStencil, const RALDepthStencilViewDesc& desc) = 0;

    // 创建深度/模板缓冲区
    virtual IRALDepthStencil* CreateDepthStencil(uint32_t width, uint32_t height, RALDataFormat format) = 0;
};
#endif//IRAL_DEVICE_H