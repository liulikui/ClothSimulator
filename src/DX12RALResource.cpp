#include "DX12RALResource.h"
#include "DX12RALDevice.h"
#include "RALCommandList.h"
#include "DX12RALCommandList.h"
#include <d3d12.h>
#include <iostream>

// 声明全局设备指针
extern IRALDevice* device;

// 日志函数声明
extern void logDebug(const std::string& message);

// 创建渲染目标视图描述符
D3D12_CPU_DESCRIPTOR_HANDLE DX12RALRenderTarget::CreateRenderTargetView(ID3D12Device* device, ID3D12DescriptorHeap* heap, uint32_t descriptorSize, uint32_t index)
{
    if (!device || !heap || !m_nativeResource)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE invalidHandle = {};
        return invalidHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = heap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += index * descriptorSize;

    device->CreateRenderTargetView(m_nativeResource.Get(), nullptr, rtvHandle);

    return rtvHandle;
}

// 创建着色器资源视图描述符
D3D12_CPU_DESCRIPTOR_HANDLE DX12RALRenderTarget::CreateShaderResourceView(ID3D12Device* device, ID3D12DescriptorHeap* heap, uint32_t descriptorSize, uint32_t index)
{
    if (!device || !heap || !m_nativeResource)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE invalidHandle = {};
        return invalidHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = heap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += index * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(m_nativeResource.Get(), &srvDesc, srvHandle);

    return srvHandle;
}

// 创建深度/模板视图描述符
D3D12_CPU_DESCRIPTOR_HANDLE DX12RALDepthStencil::CreateDepthStencilView(ID3D12Device* device, ID3D12DescriptorHeap* heap, uint32_t descriptorSize, uint32_t index)
{
    if (!device || !heap || !m_nativeResource)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE invalidHandle = {};
        return invalidHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = heap->GetCPUDescriptorHandleForHeapStart();
    dsvHandle.ptr += index * descriptorSize;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = static_cast<DXGI_FORMAT>(GetFormat());
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(m_nativeResource.Get(), &dsvDesc, dsvHandle);

    return dsvHandle;
}