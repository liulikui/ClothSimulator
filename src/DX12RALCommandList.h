#pragma once

#include "RALCommandList.h"
#include <d3d12.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

// DX12RALCommandList类
class DX12RALCommandList : public IRALCommandList
{
public:
    // 构造函数和析构函数
    DX12RALCommandList();
    virtual ~DX12RALCommandList();

    // 从IRALCommandList继承的方法
    virtual void ResourceBarrier(const RALResourceBarrier& barrier) override;
    virtual void ResourceBarriers(const RALResourceBarrier* barriers, uint32_t count) override;
    
    // 关闭命令列表（准备执行）
    virtual void Close() override;

    // 重置命令列表（重新开始录制）
    virtual void Reset() override;

    // 获取原生命令列表指针
    virtual void* GetNativeCommandList() override;

protected:
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
};

// DX12RALGraphicsCommandList类
class DX12RALGraphicsCommandList : public IRALGraphicsCommandList
{
public:
    // 构造函数和析构函数
    DX12RALGraphicsCommandList(ID3D12CommandAllocator* dx12CommandAllocator, ID3D12GraphicsCommandList* dx12CommandList);
    virtual ~DX12RALGraphicsCommandList();

    // 从IRALCommandList继承的方法
    virtual void ResourceBarrier(const RALResourceBarrier& barrier) override;
    virtual void ResourceBarriers(const RALResourceBarrier* barriers, uint32_t count) override;
    virtual void Close() override;
    virtual void Reset() override;
    virtual void* GetNativeCommandList() override;

    // 从IRALGraphicsCommandList继承的方法
    virtual void ClearRenderTarget(IRALRenderTarget* renderTarget, const float color[4]) override;
    virtual void ClearDepthStencil(IRALDepthStencil* depthStencil, float depth, uint8_t stencil) override;
    virtual void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;
    virtual void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;
    virtual void SetPipelineState(IRALResource* pipelineState) override;
    virtual void SetVertexBuffers(uint32_t startSlot, uint32_t count, IRALVertexBuffer** ppVertexBuffers) override;
    virtual void SetIndexBuffer(IRALIndexBuffer* indexBuffer) override;
    virtual void SetGraphicsRootSignature(IRALRootSignature* rootSignature) override;
    virtual void SetGraphicsRootConstant(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t value) override;
    virtual void SetGraphicsRootConstants(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t count, const uint32_t* values) override;
    virtual void SetGraphicsRootDescriptorTable(uint32_t rootParameterIndex, void* descriptorTable) override;
    virtual void SetGraphicsRootConstantBuffer(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer) override;
    virtual void SetGraphicsRootShaderResource(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer) override;
    virtual void SetGraphicsRootUnorderedAccess(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer) override;
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) override;
    virtual void DrawIndirect(void* bufferLocation, uint32_t drawCount, uint32_t stride) override;
    virtual void DrawIndexedIndirect(void* bufferLocation, uint32_t drawCount, uint32_t stride) override;
    virtual void SetRenderTargets(uint32_t renderTargetCount, IRALRenderTarget** renderTargets, IRALDepthStencil* depthStencil) override;
    // 执行渲染通道
    virtual void ExecuteRenderPass(const void* renderPass, const void* framebuffer) override;

    // 设置图元拓扑
    virtual void SetPrimitiveTopology(RALPrimitiveTopologyType topology) override;
    
    // 资源状态转换方法
    virtual void TransitionRenderTargetToShaderResource(IRALRenderTarget* renderTarget) override;
    virtual void TransitionRenderTargetToRenderTarget(IRALRenderTarget* renderTarget) override;
    virtual void TransitionDepthStencilToShaderResource(IRALDepthStencil* depthStencil) override;
    virtual void TransitionDepthStencilToDepthStencil(IRALDepthStencil* depthStencil) override;

private:
    // 成员变量
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
};