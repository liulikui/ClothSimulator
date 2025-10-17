#include "DX12RALCommandList.h"
#include "DX12RALResource.h"
#include <vector>

// 前向声明
D3D12_RESOURCE_STATES ConvertToDX12ResourceState(RALResourceState state);
D3D_PRIMITIVE_TOPOLOGY ConvertToDX12PrimitiveTopology(RALPrimitiveTopologyType topology);

// DX12RALGraphicsCommandList构造函数
DX12RALGraphicsCommandList::DX12RALGraphicsCommandList(ID3D12CommandAllocator* commandAllocator, ID3D12GraphicsCommandList* commandList)
    : IRALGraphicsCommandList()
    , m_commandAllocator(commandAllocator)
    , m_commandList(commandList)
{    
}

// DX12RALGraphicsCommandList析构函数
DX12RALGraphicsCommandList::~DX12RALGraphicsCommandList()
{
    // 释放资源，如果需要的话
}

// 资源屏障操作
void DX12RALGraphicsCommandList::ResourceBarrier(const RALResourceBarrier& barrier)
{
    ResourceBarriers(&barrier, 1);
}

// 资源屏障操作（多个）
void DX12RALGraphicsCommandList::ResourceBarriers(const RALResourceBarrier* barriers, uint32_t count)
{
    std::vector<D3D12_RESOURCE_BARRIER> dxBarriers;
    dxBarriers.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        const RALResourceBarrier& barrier = barriers[i];
        D3D12_RESOURCE_BARRIER dxBarrier = {};
        
        switch (barrier.type)
        {
        case RALResourceBarrierType::Transition:
            dxBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            dxBarrier.Transition.pResource = static_cast<ID3D12Resource*>(barrier.resource->GetNativeResource());
            dxBarrier.Transition.StateBefore = ConvertToDX12ResourceState(barrier.oldState);
            dxBarrier.Transition.StateAfter = ConvertToDX12ResourceState(barrier.newState);
            dxBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            break;
        case RALResourceBarrierType::Aliasing:
            dxBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
            dxBarrier.Aliasing.pResourceBefore = static_cast<ID3D12Resource*>(barrier.resource->GetNativeResource());
            dxBarrier.Aliasing.pResourceAfter = nullptr;
            break;
        case RALResourceBarrierType::UnorderedAccess:
            dxBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            dxBarrier.UAV.pResource = static_cast<ID3D12Resource*>(barrier.resource->GetNativeResource());
            break;
        default:
            break;
        }
        
        dxBarriers.push_back(dxBarrier);
    }
    
    m_commandList->ResourceBarrier((UINT)count, dxBarriers.data());
}

// 关闭命令列表（准备执行）
void DX12RALGraphicsCommandList::Close()
{
    m_commandList->Close();
}

// 重置命令列表（重新开始录制）
void DX12RALGraphicsCommandList::Reset()
{
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);
}

// 获取原生命令列表指针
void* DX12RALGraphicsCommandList::GetNativeCommandList()
{
    return m_commandList.Get();
}

// 清除渲染目标
void DX12RALGraphicsCommandList::ClearRenderTarget(IRALRenderTargetView* renderTargetView, const float color[4])
{
    if (!renderTargetView)
        return;
    
    DX12RALRenderTargetView* dx12RTV = static_cast<DX12RALRenderTargetView*>(renderTargetView);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx12RTV->GetRTVCPUHandle();
    m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
}

// 清除深度/模板视图
void DX12RALGraphicsCommandList::ClearDepthStencil(IRALDepthStencilView* depthStencilView, RALClearFlags clearFlags, float depth, uint8_t stencil)
{
    if (!depthStencilView)
        return;
    
    DX12RALDepthStencilView* dx12DSV = static_cast<DX12RALDepthStencilView*>(depthStencilView);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dx12DSV->GetDSVCPUHandle();
    
    // 将RALClearFlags转换为D3D12_CLEAR_FLAGS
    D3D12_CLEAR_FLAGS d3dClearFlags = (D3D12_CLEAR_FLAGS)0;
    if ((clearFlags & RALClearFlags::Depth) != RALClearFlags::None)
    {
        d3dClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
    }
    if ((clearFlags & RALClearFlags::Stencil) != RALClearFlags::None)
    {
        d3dClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
    }
    
    m_commandList->ClearDepthStencilView(dsvHandle, d3dClearFlags, depth, stencil, 0U, nullptr);
}

// 设置视口
void DX12RALGraphicsCommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = minDepth;
    viewport.MaxDepth = maxDepth;
    m_commandList->RSSetViewports(1, &viewport);
}

// 设置裁剪矩形
void DX12RALGraphicsCommandList::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    D3D12_RECT scissorRect = {};
    scissorRect.left = left;
    scissorRect.top = top;
    scissorRect.right = right;
    scissorRect.bottom = bottom;
    m_commandList->RSSetScissorRects(1, &scissorRect);
}

// 设置管线状态
void DX12RALGraphicsCommandList::SetPipelineState(IRALResource* pipelineState)
{
    if (pipelineState)
    {
        DX12RALGraphicsPipelineState* dx12PipelineState = static_cast<DX12RALGraphicsPipelineState*>(pipelineState);
        ID3D12PipelineState* nativePipelineState = static_cast<ID3D12PipelineState*>(dx12PipelineState->GetNativeResource());
        m_commandList->SetPipelineState(nativePipelineState);
    }
}

// 绑定顶点缓冲区
void DX12RALGraphicsCommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count, IRALVertexBuffer** ppVertexBuffers)
{
    std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferViews;
    bufferViews.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        D3D12_VERTEX_BUFFER_VIEW view = (static_cast<DX12RALVertexBuffer*>(ppVertexBuffers[i]))->GetVertexBufferView();
        bufferViews.push_back(view);
    }
    
    m_commandList->IASetVertexBuffers(startSlot, bufferViews.size(), bufferViews.data());
}

// 绑定索引缓冲区
void DX12RALGraphicsCommandList::SetIndexBuffer(IRALIndexBuffer* indexBuffer)
{
    if (indexBuffer)
    {
        D3D12_INDEX_BUFFER_VIEW view = (static_cast<DX12RALIndexBuffer*>(indexBuffer))->GetIndexBufferView();
        m_commandList->IASetIndexBuffer(&view);
    }
    else
    {
        m_commandList->IASetIndexBuffer(nullptr);
    }
}

// 绑定根签名
void DX12RALGraphicsCommandList::SetGraphicsRootSignature(IRALRootSignature* rootSignature)
{
    if (rootSignature)
    {
        ID3D12RootSignature* dxRootSignature = static_cast<ID3D12RootSignature*>(rootSignature->GetNativeResource());
        m_commandList->SetGraphicsRootSignature(dxRootSignature);
    }
}

// 绑定根常量
void DX12RALGraphicsCommandList::SetGraphicsRootConstant(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t value)
{
    m_commandList->SetGraphicsRoot32BitConstant(rootParameterIndex, value, shaderRegister);
}

void DX12RALGraphicsCommandList::SetGraphicsRootConstants(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t count, const uint32_t* values)
{
    m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, count, values, shaderRegister);
}

// 绑定根描述符表
void DX12RALGraphicsCommandList::SetGraphicsRootDescriptorTable(uint32_t rootParameterIndex, void* descriptorTable)
{
    if (descriptorTable)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
        handle.ptr = reinterpret_cast<uint64_t>(descriptorTable);
        m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle);
    }
}

// 绑定根描述符表（通过SRV）
void DX12RALGraphicsCommandList::SetGraphicsRootDescriptorTable(uint32_t rootParameterIndex, IRALShaderResourceView* srv)
{
    if (srv)
    {
        // 将IRALShaderResourceView转换为DX12RALShaderResourceView以获取GPU描述符句柄
        DX12RALShaderResourceView* dx12SRV = static_cast<DX12RALShaderResourceView*>(srv);
        if (dx12SRV)
        {
            ID3D12DescriptorHeap* srvHeap = dx12SRV->GetSRVHeap();
            ID3D12DescriptorHeap* heaps[1];
            heaps[0] = srvHeap;

            m_commandList->SetDescriptorHeaps(1, heaps); // 绑定堆

            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dx12SRV->GetSRVGPUHandle();
            m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuHandle);
        }
    }
}

// 绑定根常量缓冲区视图
void DX12RALGraphicsCommandList::SetGraphicsRootConstantBuffer(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer)
{
    DX12RALConstBuffer* dx12ConstBuffer = (DX12RALConstBuffer*)constBuffer;
    m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, dx12ConstBuffer->GetGPUVirtualAddress());
}

// 绑定根着色器资源视图（常量缓冲区）
void DX12RALGraphicsCommandList::SetGraphicsRootShaderResource(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer)
{
    DX12RALConstBuffer* dx12ConstBuffer = (DX12RALConstBuffer*)constBuffer;
    m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, dx12ConstBuffer->GetGPUVirtualAddress());
}

// 注意：已删除SetGraphicsRootShaderResource(IRALShaderResourceView*)方法实现

// 绑定根无序访问视图
void DX12RALGraphicsCommandList::SetGraphicsRootUnorderedAccess(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer)
{
    DX12RALConstBuffer* dx12ConstBuffer = (DX12RALConstBuffer*)constBuffer;
    m_commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, dx12ConstBuffer->GetGPUVirtualAddress());
}

// 绘制调用（无索引）
void DX12RALGraphicsCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    m_commandList->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}

// 绘制调用（有索引）
void DX12RALGraphicsCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    m_commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

// 绘制调用（间接）
void DX12RALGraphicsCommandList::DrawIndirect(void* bufferLocation, uint32_t drawCount, uint32_t stride)
{
    // DirectX 12中没有直接的DrawIndirect方法，需要使用ExecuteIndirect
    // 这里简化处理，实际项目中需要创建命令签名
    // 注意：这个实现是简化版本，实际项目中需要完整实现ExecuteIndirect的逻辑
    if (bufferLocation)
    {
        // 暂时不实现，返回警告或错误
        // 实际实现需要：
        // 1. 创建命令签名 (ID3D12CommandSignature)
        // 2. 设置正确的参数缓冲区
        // 3. 调用ExecuteIndirect方法
    }
}

// 绘制调用（索引间接）
void DX12RALGraphicsCommandList::DrawIndexedIndirect(void* bufferLocation, uint32_t drawCount, uint32_t stride)
{
    // DirectX 12中没有直接的DrawIndexedIndirect方法，需要使用ExecuteIndirect
    // 这里简化处理，实际项目中需要创建命令签名
    // 注意：这个实现是简化版本，实际项目中需要完整实现ExecuteIndirect的逻辑
    if (bufferLocation)
    {
        // 暂时不实现，返回警告或错误
        // 实际实现需要：
        // 1. 创建命令签名 (ID3D12CommandSignature)
        // 2. 设置正确的参数缓冲区
        // 3. 调用ExecuteIndirect方法
    }
}

// 设置渲染目标
void DX12RALGraphicsCommandList::SetRenderTargets(uint32_t renderTargetCount, IRALRenderTargetView** renderTargetViews, IRALDepthStencilView* depthStencilView)
{
    // 确保渲染目标数量不超过D3D12支持的最大数量（通常为8）
    const uint32_t kMaxRenderTargets = 8;
    if (renderTargetCount > kMaxRenderTargets)
    {
        renderTargetCount = kMaxRenderTargets;
    }
    
    // 创建描述符句柄数组
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles(renderTargetCount);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
    
    // 获取渲染目标视图描述符
    for (uint32_t i = 0; i < renderTargetCount; ++i)
    {
        if (renderTargetViews[i])
        {
            DX12RALRenderTargetView* dx12Rtv = static_cast<DX12RALRenderTargetView*>(renderTargetViews[i]);
            rtvHandles[i] = dx12Rtv->GetRTVCPUHandle();
        }
    }
    
    if (depthStencilView)
    {
        DX12RALDepthStencilView* dx12Dsv = static_cast<DX12RALDepthStencilView*>(depthStencilView);
        dsvHandle = dx12Dsv->GetDSVCPUHandle();
    }
    
    m_commandList->OMSetRenderTargets(
        renderTargetCount,
        renderTargetCount > 0 ? rtvHandles.data() : nullptr,
        FALSE, // 不绑定到所有视图
        depthStencilView ? &dsvHandle : nullptr
    );
}

// 执行渲染通道
void DX12RALGraphicsCommandList::ExecuteRenderPass(const void* renderPass, const void* framebuffer)
{
    // DX12中没有直接的渲染通道概念，这个函数需要根据项目需求实现
    // 这里不做具体实现，因为需要更多的项目上下文
}

// 设置图元拓扑
void DX12RALGraphicsCommandList::SetPrimitiveTopology(RALPrimitiveTopologyType topology)
{
    D3D_PRIMITIVE_TOPOLOGY dxTopology = ConvertToDX12PrimitiveTopology(topology);
    m_commandList->IASetPrimitiveTopology(dxTopology);
}
