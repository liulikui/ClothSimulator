#ifndef RALCOMMANDLIST_H
#define RALCOMMANDLIST_H

#include "RALResource.h"
#include <atomic>

// 命令列表类型枚举
enum class RALCommandListType
{
    Graphics,
    Compute,
    Copy
};

// 资源屏障类型
enum class RALResourceBarrierType
{
    Transition,
    Aliasing,
    UnorderedAccess
};

// 资源状态
enum class RALResourceState
{
    Common,
    VertexAndConstantBuffer,
    IndexBuffer,
    RenderTarget,
    UnorderedAccess,
    DepthWrite,
    DepthRead,
    ShaderResource,
    StreamOut,
    IndirectArgument,
    CopyDest,
    CopySource,
    ResolveDest,
    ResolveSource
};

// 资源屏障结构体
struct RALResourceBarrier
{
    RALResourceBarrierType type;
    IRALResource* resource;
    RALResourceState oldState;
    RALResourceState newState;
};

// 命令列表抽象基类
class IRALCommandList
{
public:
    IRALCommandList(RALCommandListType type)
        : m_type(type)
    {
    }

    virtual ~IRALCommandList() = default;

    // 增加引用计数
    void AddRef()
    {
        m_refCount.fetch_add(1, std::memory_order_acquire);
    }

    // 减少引用计数
    void Release()
    {
        if (m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }

    // 获取命令列表类型
    RALCommandListType GetType() const { return m_type; }

    // 资源屏障操作
    virtual void ResourceBarrier(const RALResourceBarrier& barrier) = 0;
    virtual void ResourceBarriers(const RALResourceBarrier* barriers, uint32_t count) = 0;

    // 关闭命令列表（准备执行）
    virtual void Close() = 0;

    // 重置命令列表（重新开始录制）
    virtual void Reset() = 0;

    // 获取原生命令列表指针
    virtual void* GetNativeCommandList() = 0;

protected:
    RALCommandListType m_type;
    std::atomic<int32_t> m_refCount;
};

// 图形命令列表接口
class IRALGraphicsCommandList : public IRALCommandList
{
public:
    IRALGraphicsCommandList()
        : IRALCommandList(RALCommandListType::Graphics)
    {
    }

    virtual ~IRALGraphicsCommandList() = default;

    // 清除渲染目标
    virtual void ClearRenderTargetView(IRALRenderTargetView* renderTargetView, const float color[4]) = 0;

    // 清除深度/模板视图
    virtual void ClearDepthStencilView(IRALDepthStencilView* depthStencilView, float depth, uint8_t stencil) = 0;

    // 设置视口
    virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;

    // 设置裁剪矩形
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) = 0;

    // 设置管线状态
    virtual void SetPipelineState(IRALResource* pipelineState) = 0;

    // 绑定顶点缓冲区
    virtual void SetVertexBuffers(uint32_t startSlot, uint32_t count, IRALVertexBufferView** vertexBufferViews) = 0;

    // 绑定索引缓冲区
    virtual void SetIndexBuffer(IRALIndexBufferView* indexBufferView) = 0;

    // 绑定根签名
    virtual void SetGraphicsRootSignature(IRALRootSignature* rootSignature) = 0;

    // 绑定根常量
    virtual void SetGraphicsRootConstant(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t value) = 0;
    virtual void SetGraphicsRootConstants(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t count, const uint32_t* values) = 0;

    // 绑定根描述符表
    virtual void SetGraphicsRootDescriptorTable(uint32_t rootParameterIndex, void* descriptorTable) = 0;

    // 绑定根常量缓冲区视图
    virtual void SetGraphicsRootConstantBuffer(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer) = 0;

    // 绑定根着色器资源视图
    virtual void SetGraphicsRootShaderResource(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer) = 0;

    // 绑定根无序访问视图
    virtual void SetGraphicsRootUnorderedAccess(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer) = 0;

    // 绘制调用（无索引）
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0) = 0;

    // 绘制调用（有索引）
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndexLocation = 0, int32_t baseVertexLocation = 0, uint32_t startInstanceLocation = 0) = 0;

    // 绘制调用（间接）
    virtual void DrawIndirect(void* bufferLocation, uint32_t drawCount, uint32_t stride) = 0;

    // 绘制调用（索引间接）
    virtual void DrawIndexedIndirect(void* bufferLocation, uint32_t drawCount, uint32_t stride) = 0;

    // 设置渲染目标
    virtual void SetRenderTargets(uint32_t renderTargetCount, IRALRenderTargetView** renderTargetViews, IRALDepthStencilView* depthStencilView = nullptr) = 0;

    // 执行渲染通道
    virtual void ExecuteRenderPass(const void* renderPass, const void* framebuffer) = 0;
};

#endif // RALCOMMANDLIST_H