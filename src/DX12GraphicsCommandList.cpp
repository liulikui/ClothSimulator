#include "DX12GraphicsCommandList.h"
#include <stdexcept>

// DX12GraphicsCommandList构造函数
DX12GraphicsCommandList::DX12GraphicsCommandList(ID3D12GraphicsCommandList* commandList)
    : commandList_(commandList) {
    if (!commandList) {
        throw std::runtime_error("Invalid command list pointer");
    }
}

// 实现资源状态转换
void DX12GraphicsCommandList::ResourceBarrier(uint32_t numBarriers, const void* pBarriers) {
    if (commandList_ && pBarriers) {
        commandList_->ResourceBarrier(numBarriers, static_cast<const D3D12_RESOURCE_BARRIER*>(pBarriers));
    }
}

// 实现设置渲染目标视图
void DX12GraphicsCommandList::OMSetRenderTargets(uint32_t numRenderTargets, const void* renderTargetViews, 
                                                bool rtvsSingleHandleToDescriptorRange, const void* depthStencilView) {
    if (commandList_) {
        commandList_->OMSetRenderTargets(
            numRenderTargets,
            static_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(renderTargetViews),
            rtvsSingleHandleToDescriptorRange,
            static_cast<const D3D12_CPU_DESCRIPTOR_HANDLE*>(depthStencilView)
        );
    }
}

// 实现设置视口
void DX12GraphicsCommandList::RSSetViewports(uint32_t numViewports, const void* viewports) {
    if (commandList_ && viewports) {
        commandList_->RSSetViewports(numViewports, static_cast<const D3D12_VIEWPORT*>(viewports));
    }
}

// 实现设置裁剪矩形
void DX12GraphicsCommandList::RSSetScissorRects(uint32_t numRects, const void* rects) {
    if (commandList_ && rects) {
        commandList_->RSSetScissorRects(numRects, static_cast<const D3D12_RECT*>(rects));
    }
}

// 实现设置管道状态
void DX12GraphicsCommandList::SetPipelineState(const void* pipelineState) {
    if (commandList_ && pipelineState) {
        commandList_->SetPipelineState(static_cast<ID3D12PipelineState*>(const_cast<void*>(pipelineState)));
    }
}

// 实现设置顶点缓冲区
void DX12GraphicsCommandList::IASetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, const void* vertexBufferViews) {
    if (commandList_ && vertexBufferViews) {
        commandList_->IASetVertexBuffers(
            startSlot,
            numBuffers,
            static_cast<const D3D12_VERTEX_BUFFER_VIEW*>(vertexBufferViews)
        );
    }
}

// 实现设置索引缓冲区
void DX12GraphicsCommandList::IASetIndexBuffer(const void* indexBufferView) {
    if (commandList_ && indexBufferView) {
        commandList_->IASetIndexBuffer(static_cast<const D3D12_INDEX_BUFFER_VIEW*>(indexBufferView));
    }
}

// 实现设置图元拓扑
void DX12GraphicsCommandList::IASetPrimitiveTopology(uint32_t topology) {
    if (commandList_) {
        commandList_->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
    }
}

// 实现绘制索引图元
void DX12GraphicsCommandList::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, 
                                                 uint32_t startIndexLocation, int32_t baseVertexLocation, 
                                                 uint32_t startInstanceLocation) {
    if (commandList_) {
        commandList_->DrawIndexedInstanced(
            indexCountPerInstance,
            instanceCount,
            startIndexLocation,
            baseVertexLocation,
            startInstanceLocation
        );
    }
}

// 实现绘制非索引图元
void DX12GraphicsCommandList::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, 
                                          uint32_t startVertexLocation, uint32_t startInstanceLocation) {
    if (commandList_) {
        commandList_->DrawInstanced(
            vertexCountPerInstance,
            instanceCount,
            startVertexLocation,
            startInstanceLocation
        );
    }
}

// 实现关闭命令列表
void DX12GraphicsCommandList::Close() {
    if (commandList_) {
        commandList_->Close();
    }
}

// 实现获取底层命令列表接口
void* DX12GraphicsCommandList::GetNativeCommandList() {
    return commandList_;
}