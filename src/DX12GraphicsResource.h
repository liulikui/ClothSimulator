#pragma once
#include "IGraphicsResource.h"
#include <d3d12.h>
#include <wrl.h>

namespace wrl = Microsoft::WRL;

class DX12VertexBufferView : public IVertexBufferView {
public:
    explicit DX12VertexBufferView(const D3D12_VERTEX_BUFFER_VIEW& view)
        : m_view(view) {}
    
    ~DX12VertexBufferView() override = default;
    
    uint64_t GetBufferLocation() const override { return m_view.BufferLocation; }
    uint32_t GetSizeInBytes() const override { return m_view.SizeInBytes; }
    uint32_t GetStrideInBytes() const override { return m_view.StrideInBytes; }
    void* GetNativeView() override { return &m_view; }
    
private:
    D3D12_VERTEX_BUFFER_VIEW m_view;
};

class DX12IndexBufferView : public IIndexBufferView {
public:
    DX12IndexBufferView(const D3D12_INDEX_BUFFER_VIEW& view, bool is32Bit)
        : m_view(view), m_is32Bit(is32Bit) {}
    
    ~DX12IndexBufferView() override = default;
    
    uint64_t GetBufferLocation() const override { return m_view.BufferLocation; }
    uint32_t GetSizeInBytes() const override { return m_view.SizeInBytes; }
    bool Is32Bit() const override { return m_is32Bit; }
    void* GetNativeView() override { return &m_view; }
    
private:
    D3D12_INDEX_BUFFER_VIEW m_view;
    bool m_is32Bit;
};

class DX12VertexBuffer : public IVertexBuffer {
public:
    DX12VertexBuffer(ID3D12Resource* buffer, uint32_t stride, uint32_t sizeInBytes)
        : m_buffer(buffer), m_stride(stride), m_sizeInBytes(sizeInBytes) {}
    
    ~DX12VertexBuffer() override = default;
    
    void* GetNativeResource() override { return m_buffer.Get(); }
    
    IVertexBufferView* CreateVertexBufferView(uint32_t strideParam, uint32_t sizeInBytesParam) override {
        D3D12_VERTEX_BUFFER_VIEW view = {};
        view.BufferLocation = m_buffer->GetGPUVirtualAddress();
        view.SizeInBytes = (sizeInBytesParam > 0) ? sizeInBytesParam : m_sizeInBytes;
        view.StrideInBytes = (strideParam > 0) ? strideParam : m_stride;
        
        return new DX12VertexBufferView(view);
    }
    
private:
    wrl::ComPtr<ID3D12Resource> m_buffer;
    uint32_t m_stride;
    uint32_t m_sizeInBytes;
};

class DX12IndexBuffer : public IIndexBuffer {
public:
    DX12IndexBuffer(ID3D12Resource* buffer, uint32_t sizeInBytes, bool is32Bit)
        : m_buffer(buffer), m_sizeInBytes(sizeInBytes), m_is32Bit(is32Bit) {}
    
    ~DX12IndexBuffer() override = default;
    
    void* GetNativeResource() override { return m_buffer.Get(); }
    
    IIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytesParam, bool is32BitParam) override {
        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = m_buffer->GetGPUVirtualAddress();
        view.SizeInBytes = (sizeInBytesParam > 0) ? sizeInBytesParam : m_sizeInBytes;
        view.Format = is32BitParam ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        
        return new DX12IndexBufferView(view, is32BitParam);
    }
    
private:
    wrl::ComPtr<ID3D12Resource> m_buffer;
    uint32_t m_sizeInBytes;
    bool m_is32Bit;
};