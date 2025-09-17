#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <DirectXMath.h>

// 简化命名空间
namespace dx = DirectX;

// 前向声明
class ID3D12GraphicsCommandList;

// 图形命令列表接口
class IGraphicsCommandList {
public:
    virtual ~IGraphicsCommandList() = default;

    // 资源状态转换
    virtual void ResourceBarrier(uint32_t numBarriers, const void* pBarriers) = 0;

    // 设置渲染目标视图
    virtual void OMSetRenderTargets(uint32_t numRenderTargets, const void* renderTargetViews, 
                                   bool rtvsSingleHandleToDescriptorRange, const void* depthStencilView) = 0;

    // 设置视口
    virtual void RSSetViewports(uint32_t numViewports, const void* viewports) = 0;

    // 设置裁剪矩形
    virtual void RSSetScissorRects(uint32_t numRects, const void* rects) = 0;

    // 设置管道状态
    virtual void SetPipelineState(const void* pipelineState) = 0;

    // 设置顶点缓冲区
    virtual void IASetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const void* vertexBufferViews) = 0;

    // 设置索引缓冲区
    virtual void IASetIndexBuffer(const void* indexBufferView) = 0;

    // 设置图元拓扑
    virtual void IASetPrimitiveTopology(uint32_t topology) = 0;

    // 绘制索引图元
    virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, 
                                     uint32_t startIndexLocation, int32_t baseVertexLocation, 
                                     uint32_t startInstanceLocation) = 0;

    // 绘制非索引图元
    virtual void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, 
                              uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;

    // 关闭命令列表
    virtual void Close() = 0;

    // 获取底层命令列表接口
    virtual void* GetNativeCommandList() = 0;
};

// DX12实现的图形命令列表
class DX12GraphicsCommandList : public IGraphicsCommandList {
public:
    DX12GraphicsCommandList(ID3D12GraphicsCommandList* commandList);
    ~DX12GraphicsCommandList() override = default;

    // 实现IGraphicsCommandList接口方法
    void ResourceBarrier(uint32_t numBarriers, const void* pBarriers) override;
    void OMSetRenderTargets(uint32_t numRenderTargets, const void* renderTargetViews, 
                           bool rtvsSingleHandleToDescriptorRange, const void* depthStencilView) override;
    void RSSetViewports(uint32_t numViewports, const void* viewports) override;
    void RSSetScissorRects(uint32_t numRects, const void* rects) override;
    void SetPipelineState(const void* pipelineState) override;
    void IASetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const void* vertexBufferViews) override;
    void IASetIndexBuffer(const void* indexBufferView) override;
    void IASetPrimitiveTopology(uint32_t topology) override;
    void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, 
                             uint32_t startIndexLocation, int32_t baseVertexLocation, 
                             uint32_t startInstanceLocation) override;
    void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, 
                      uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
    void Close() override;
    void* GetNativeCommandList() override;

private:
    ID3D12GraphicsCommandList* commandList_; // 底层D3D12命令列表
};