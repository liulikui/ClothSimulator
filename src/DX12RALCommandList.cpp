#include "DX12RALCommandList.h"
#include <vector>

// DX12RALGraphicsCommandList构造函数
DX12RALGraphicsCommandList::DX12RALGraphicsCommandList(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandAllocator> commandAllocator)
    : IRALGraphicsCommandList()
    , m_device(device)
    , m_commandAllocator(commandAllocator)
{
    // 创建命令列表
    HRESULT hr = m_device->CreateCommandList(
        0,                              // 节点掩码
        D3D12_COMMAND_LIST_TYPE_DIRECT, // 命令列表类型
        m_commandAllocator.Get(),       // 命令分配器
        nullptr,                        // 初始管道状态对象
        IID_PPV_ARGS(&m_commandList)
    );
    if (FAILED(hr)) {
        // 处理错误，实际项目中应添加适当的错误处理
    }
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
void DX12RALGraphicsCommandList::ClearRenderTargetView(IRALRenderTargetView* renderTargetView, const float color[4])
{
    if (renderTargetView)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
        rtvHandle.ptr = reinterpret_cast<uint64_t>(renderTargetView->GetNativeRenderTargetView());
        m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
    }
}

// 清除深度/模板视图
void DX12RALGraphicsCommandList::ClearDepthStencilView(IRALDepthStencilView* depthStencilView, float depth, uint8_t stencil)
{
    if (depthStencilView)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
        dsvHandle.ptr = reinterpret_cast<uint64_t>(depthStencilView->GetNativeDepthStencilView());
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
    }
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
        ID3D12PipelineState* dxPipelineState = static_cast<ID3D12PipelineState*>(pipelineState->GetNativeResource());
        m_commandList->SetPipelineState(dxPipelineState);
    }
}

// 绑定顶点缓冲区
void DX12RALGraphicsCommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count, IRALVertexBufferView** vertexBufferViews)
{
    std::vector<D3D12_VERTEX_BUFFER_VIEW> bufferViews;
    bufferViews.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        if (vertexBufferViews[i])
        {
            D3D12_VERTEX_BUFFER_VIEW view = {};
            view.BufferLocation = vertexBufferViews[i]->GetBufferLocation();
            view.SizeInBytes = vertexBufferViews[i]->GetSizeInBytes();
            view.StrideInBytes = 0; // 需要从某个地方获取步长，这里暂时设为0
            bufferViews.push_back(view);
        }
    }
    
    m_commandList->IASetVertexBuffers(startSlot, bufferViews.size(), bufferViews.data());
}

// 绑定索引缓冲区
void DX12RALGraphicsCommandList::SetIndexBuffer(IRALIndexBufferView* indexBufferView)
{
    if (indexBufferView)
    {
        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = indexBufferView->GetBufferLocation();
        view.SizeInBytes = indexBufferView->GetSizeInBytes();
        view.Format = indexBufferView->Is32Bit() ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        m_commandList->IASetIndexBuffer(&view);
    }
    else
    {
        m_commandList->IASetIndexBuffer(nullptr);
    }
}

// 绑定根签名
void DX12RALGraphicsCommandList::SetRootSignature(IRALRootSignature* rootSignature)
{
    if (rootSignature)
    {
        ID3D12RootSignature* dxRootSignature = static_cast<ID3D12RootSignature*>(rootSignature->GetNativeResource());
        m_commandList->SetGraphicsRootSignature(dxRootSignature);
    }
}

// 绑定根常量
void DX12RALGraphicsCommandList::SetRootConstant(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t value)
{
    m_commandList->SetGraphicsRoot32BitConstant(rootParameterIndex, value, shaderRegister);
}

void DX12RALGraphicsCommandList::SetRootConstants(uint32_t rootParameterIndex, uint32_t shaderRegister, uint32_t count, const uint32_t* values)
{
    m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, count, values, shaderRegister);
}

// 绑定根描述符表
void DX12RALGraphicsCommandList::SetRootDescriptorTable(uint32_t rootParameterIndex, void* descriptorTable)
{
    if (descriptorTable)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
        handle.ptr = reinterpret_cast<uint64_t>(descriptorTable);
        m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle);
    }
}

// 绑定根常量缓冲区视图
void DX12RALGraphicsCommandList::SetRootConstantBufferView(uint32_t rootParameterIndex, uint64_t bufferLocation)
{
    m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferLocation);
}

// 绑定根着色器资源视图
void DX12RALGraphicsCommandList::SetRootShaderResourceView(uint32_t rootParameterIndex, uint64_t bufferLocation)
{
    m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, bufferLocation);
}

// 绑定根无序访问视图
void DX12RALGraphicsCommandList::SetRootUnorderedAccessView(uint32_t rootParameterIndex, uint64_t bufferLocation)
{
    m_commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, bufferLocation);
}

// 创建UniformBuffer
IRALUniformBuffer* DX12RALGraphicsCommandList::CreateUniformBuffer(uint32_t sizeInBytes)
{
    // 注意：这个函数在实际项目中需要实现，创建一个DX12RALUniformBuffer对象
    // 这里返回nullptr是为了编译通过，实际实现需要完成
    return nullptr;
}

// 更新UniformBuffer数据
void DX12RALGraphicsCommandList::UpdateUniformBuffer(IRALUniformBuffer* buffer, const void* data, uint32_t sizeInBytes)
{
    if (buffer)
    {
        buffer->Update(data, sizeInBytes);
    }
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
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
    rtvHandles.reserve(renderTargetCount);

    for (uint32_t i = 0; i < renderTargetCount; ++i) {
        if (renderTargetViews[i]) {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
            handle.ptr = reinterpret_cast<uint64_t>(renderTargetViews[i]->GetNativeRenderTargetView());
            rtvHandles.push_back(handle);
        }
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
    dsvHandle.ptr = depthStencilView ? reinterpret_cast<uint64_t>(depthStencilView->GetNativeDepthStencilView()) : 0;
    
    if (rtvHandles.empty())
    {
        m_commandList->OMSetRenderTargets(0, nullptr, FALSE, dsvHandle.ptr ? &dsvHandle : nullptr);
    }
    else
    {
        m_commandList->OMSetRenderTargets(rtvHandles.size(), rtvHandles.data(), FALSE, dsvHandle.ptr ? &dsvHandle : nullptr);
    }
}

// 执行渲染通道
void DX12RALGraphicsCommandList::ExecuteRenderPass(const void* renderPass, const void* framebuffer)
{
    // DX12中没有直接的渲染通道概念，这个函数需要根据项目需求实现
    // 这里不做具体实现，因为需要更多的项目上下文
}

// 将RAL资源状态转换为DX12资源状态
D3D12_RESOURCE_STATES DX12RALGraphicsCommandList::ConvertToDX12ResourceState(RALResourceState state)
{
    switch (state)
    {
    case RALResourceState::Common:
        return D3D12_RESOURCE_STATE_COMMON;
    case RALResourceState::VertexAndConstantBuffer:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case RALResourceState::IndexBuffer:
        return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case RALResourceState::RenderTarget:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case RALResourceState::UnorderedAccess:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case RALResourceState::DepthWrite:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case RALResourceState::DepthRead:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case RALResourceState::ShaderResource:
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case RALResourceState::StreamOut:
        return D3D12_RESOURCE_STATE_STREAM_OUT;
    case RALResourceState::IndirectArgument:
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case RALResourceState::CopyDest:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case RALResourceState::CopySource:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case RALResourceState::ResolveDest:
        return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case RALResourceState::ResolveSource:
        return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    default:
        return D3D12_RESOURCE_STATE_COMMON;
    }
}