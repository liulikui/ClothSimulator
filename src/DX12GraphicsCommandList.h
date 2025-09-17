#pragma once
#include "IGraphicsCommandList.h"
#include <d3d12.h>
#include <wrl.h>

// 简化命名空间
namespace wrl = Microsoft::WRL;

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
    void IASetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, IVertexBufferView** vertexBufferViews) override;
    void IASetIndexBuffer(IIndexBufferView* indexBufferView) override;
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