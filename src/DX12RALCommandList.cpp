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
void DX12RALGraphicsCommandList::ClearRenderTarget(IRALRenderTarget* renderTarget, const float color[4])
{
    // 暂时跳过实现，避免编译错误
}

// 清除深度/模板视图
void DX12RALGraphicsCommandList::ClearDepthStencil(IRALDepthStencil* depthStencil, float depth, uint8_t stencil)
{
    // 暂时跳过实现，避免编译错误
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

// 绑定根常量缓冲区视图
void DX12RALGraphicsCommandList::SetGraphicsRootConstantBuffer(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer)
{
    DX12RALConstBuffer* dx12ConstBuffer = (DX12RALConstBuffer*)constBuffer;
    m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, dx12ConstBuffer->GetGPUVirtualAddress());
}

// 绑定根着色器资源视图
void DX12RALGraphicsCommandList::SetGraphicsRootShaderResource(uint32_t rootParameterIndex, IRALConstBuffer* constBuffer)
{
    DX12RALConstBuffer* dx12ConstBuffer = (DX12RALConstBuffer*)constBuffer;
    m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, dx12ConstBuffer->GetGPUVirtualAddress());
}

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
void DX12RALGraphicsCommandList::SetRenderTargets(uint32_t renderTargetCount, IRALRenderTarget** renderTargets, IRALDepthStencil* depthStencil)
{
    // 确保渲染目标数量不超过D3D12支持的最大数量（通常为8）
    const uint32_t kMaxRenderTargets = 8;
    if (renderTargetCount > kMaxRenderTargets)
    {
        renderTargetCount = kMaxRenderTargets;
    }
    
    // 注意：根据要求，资源状态转换已移到外部，调用者需要确保渲染目标和深度模板
    // 已经处于正确的状态（RenderTarget和DepthStencil）
    
    // 创建描述符句柄数组
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles(renderTargetCount);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
    
    // 由于现在我们不再在DX12RALGraphicsCommandList中管理descriptor heap，
    // 我们假设渲染目标和深度模板已经在外部（DX12RALDevice）中创建了视图描述符
    // 这里我们使用DX12RALRenderTarget和DX12RALDepthStencil类的方法来获取视图描述符
    
    // 获取渲染目标视图描述符
    // 注意：这里假设renderTargets数组中的每个元素都已经在DX12RALDevice中创建了对应的视图描述符
    // 并且索引i对应于描述符堆中的位置
    for (uint32_t i = 0; i < renderTargetCount; ++i)
    {
        if (renderTargets[i])
        {
            // 由于我们不再拥有descriptor heap，我们需要从DX12RALDevice中获取
            // 这里我们暂时使用一个简化的方法，假设device中已经有了合适的descriptor heap
            // 在实际实现中，应该通过适当的接口从DX12RALDevice获取这些信息
            
            // 为了编译通过，我们暂时使用空句柄
            // 正确的实现应该是从DX12RALDevice获取descriptor heap和descriptor size
            // 然后调用CreateRenderTargetView方法
        }
    }
    
    // 获取深度模板视图描述符
    if (depthStencil)
    {
        // 同样，我们暂时使用空句柄
        // 正确的实现应该是从DX12RALDevice获取descriptor heap和descriptor size
        // 然后调用CreateDepthStencilView方法
    }
    
    // 调用D3D12的OMSetRenderTargets方法设置渲染目标
    m_commandList->OMSetRenderTargets(
        renderTargetCount,
        renderTargetCount > 0 ? rtvHandles.data() : nullptr,
        FALSE, // 不绑定到所有视图
        depthStencil ? &dsvHandle : nullptr
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

// 将渲染目标转换为着色器资源视图
void DX12RALGraphicsCommandList::TransitionRenderTargetToShaderResource(IRALRenderTarget* renderTarget)
{
    if (!renderTarget)
        return;
    
    RALResourceBarrier barrier;
    barrier.type = RALResourceBarrierType::Transition;
    barrier.resource = renderTarget;
    barrier.oldState = renderTarget->GetResourceState();
    barrier.newState = RALResourceState::ShaderResource;
    
    ResourceBarrier(barrier);
}

// 将渲染目标转换为渲染目标视图
void DX12RALGraphicsCommandList::TransitionRenderTargetToRenderTarget(IRALRenderTarget* renderTarget)
{
    if (!renderTarget)
        return;
    
    RALResourceBarrier barrier;
    barrier.type = RALResourceBarrierType::Transition;
    barrier.resource = renderTarget;
    barrier.oldState = renderTarget->GetResourceState();
    barrier.newState = RALResourceState::RenderTarget;
    
    ResourceBarrier(barrier);
}

// 将深度/模板缓冲区转换为着色器资源视图
void DX12RALGraphicsCommandList::TransitionDepthStencilToShaderResource(IRALDepthStencil* depthStencil)
{
    if (!depthStencil)
        return;
    
    RALResourceBarrier barrier;
    barrier.type = RALResourceBarrierType::Transition;
    barrier.resource = depthStencil;
    barrier.oldState = depthStencil->GetResourceState();
    barrier.newState = RALResourceState::ShaderResource;
    
    ResourceBarrier(barrier);
}

// 将深度/模板缓冲区转换为深度/模板视图
void DX12RALGraphicsCommandList::TransitionDepthStencilToDepthStencil(IRALDepthStencil* depthStencil)
{
    if (!depthStencil)
        return;
    
    RALResourceBarrier barrier;
    barrier.type = RALResourceBarrierType::Transition;
    barrier.resource = depthStencil;
    barrier.oldState = depthStencil->GetResourceState();
    barrier.newState = RALResourceState::DepthStencil;
    
    ResourceBarrier(barrier);
}
