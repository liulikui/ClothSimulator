#pragma once
#include <cstdint>
#include "IGraphicsResource.h"

// 图形命令列表接口抽象类
class IGraphicsCommandList {
public:
    virtual ~IGraphicsCommandList() = default;

    // 资源状态转换
    virtual void ResourceBarrier(uint32_t numBarriers, const void* pBarriers) = 0;

    // 设置渲染目标和深度模板视图
    virtual void OMSetRenderTargets(uint32_t numRenderTargets, const void* renderTargetViews,
                                  bool rtvsSingleHandleToDescriptorRange, const void* depthStencilView) = 0;

    // 设置视口
    virtual void RSSetViewports(uint32_t numViewports, const void* viewports) = 0;

    // 设置裁剪矩形
    virtual void RSSetScissorRects(uint32_t numRects, const void* rects) = 0;

    // 设置管道状态
    virtual void SetPipelineState(const void* pipelineState) = 0;

    // 设置顶点缓冲区
    virtual void IASetVertexBuffers(uint32_t startSlot, uint32_t numBuffers, IVertexBufferView** vertexBufferViews) = 0;

    // 设置索引缓冲区
    virtual void IASetIndexBuffer(IIndexBufferView* indexBufferView) = 0;

    // 设置图元拓扑
    virtual void IASetPrimitiveTopology(uint32_t topology) = 0;

    // 绘制索引实例化图元
    virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount,
                                   uint32_t startIndexLocation, int32_t baseVertexLocation,
                                   uint32_t startInstanceLocation) = 0;

    // 绘制非索引实例化图元
    virtual void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
                            uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;

    // 关闭命令列表
    virtual void Close() = 0;

    // 获取原生命令列表指针（用于实现细节访问）
    virtual void* GetNativeCommandList() = 0;

    // 设置图形根常量缓冲区视图
    virtual void SetGraphicsRootConstantBufferView(uint32_t rootParameterIndex, IConstBufferView* constBufferView) = 0;
};