#ifndef DX12RALRESOURCE_H
#define DX12RALRESOURCE_H

#include "RALResource.h"
#include <dxgiformat.h>
#include <d3dcommon.h>
#include <d3d12.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// 将跨平台DataFormat转换为DX12的DXGI_FORMAT
inline DXGI_FORMAT toDXGIFormat(DataFormat format) {
	switch (format) 
	{
		// 单通道8位
	case DataFormat::R8_UInt:          return DXGI_FORMAT_R8_UINT;
	case DataFormat::R8_SInt:          return DXGI_FORMAT_R8_SINT;
	case DataFormat::R8_UNorm:         return DXGI_FORMAT_R8_UNORM;
	case DataFormat::R8_SNorm:         return DXGI_FORMAT_R8_SNORM;

		// 单通道16位
	case DataFormat::R16_UInt:         return DXGI_FORMAT_R16_UINT;
	case DataFormat::R16_SInt:         return DXGI_FORMAT_R16_SINT;
	case DataFormat::R16_UNorm:        return DXGI_FORMAT_R16_UNORM;
	case DataFormat::R16_SNorm:        return DXGI_FORMAT_R16_SNORM;
	case DataFormat::R16_Float:        return DXGI_FORMAT_R16_FLOAT;

		// 单通道32位
	case DataFormat::R32_UInt:         return DXGI_FORMAT_R32_UINT;
	case DataFormat::R32_SInt:         return DXGI_FORMAT_R32_SINT;
	case DataFormat::R32_Float:        return DXGI_FORMAT_R32_FLOAT;

		// 双通道8位
	case DataFormat::R8G8_UInt:        return DXGI_FORMAT_R8G8_UINT;
	case DataFormat::R8G8_SInt:        return DXGI_FORMAT_R8G8_SINT;
	case DataFormat::R8G8_UNorm:       return DXGI_FORMAT_R8G8_UNORM;
	case DataFormat::R8G8_SNorm:       return DXGI_FORMAT_R8G8_SNORM;

		// 双通道16位
	case DataFormat::R16G16_UInt:      return DXGI_FORMAT_R16G16_UINT;
	case DataFormat::R16G16_SInt:      return DXGI_FORMAT_R16G16_SINT;
	case DataFormat::R16G16_UNorm:     return DXGI_FORMAT_R16G16_UNORM;
	case DataFormat::R16G16_SNorm:     return DXGI_FORMAT_R16G16_SNORM;
	case DataFormat::R16G16_Float:     return DXGI_FORMAT_R16G16_FLOAT;

		// 双通道32位
	case DataFormat::R32G32_UInt:      return DXGI_FORMAT_R32G32_UINT;
	case DataFormat::R32G32_SInt:      return DXGI_FORMAT_R32G32_SINT;
	case DataFormat::R32G32_Float:     return DXGI_FORMAT_R32G32_FLOAT;

		// 三通道8位格式（DXGI不原生支持，返回UNKNOWN）
	case DataFormat::R8G8B8_UInt:
	case DataFormat::R8G8B8_SInt:
	case DataFormat::R8G8B8_UNorm:
	case DataFormat::R8G8B8_SNorm:
		// 注意：DXGI不支持纯R8G8B8格式（3字节未对齐），建议使用R8G8B8A8替代
		return DXGI_FORMAT_UNKNOWN;

	   // 三通道32位
	case DataFormat::R32G32B32_Float:  return DXGI_FORMAT_R32G32B32_FLOAT;

		// 四通道8位
	case DataFormat::R8G8B8A8_UInt:    return DXGI_FORMAT_R8G8B8A8_UINT;
	case DataFormat::R8G8B8A8_SInt:    return DXGI_FORMAT_R8G8B8A8_SINT;
	case DataFormat::R8G8B8A8_UNorm:   return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DataFormat::R8G8B8A8_SNorm:   return DXGI_FORMAT_R8G8B8A8_SNORM;
	case DataFormat::R8G8B8A8_SRGB:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // SRGB对应DXGI的UNORM_SRGB

		// 四通道16位
	case DataFormat::R16G16B16A16_UInt:    return DXGI_FORMAT_R16G16B16A16_UINT;
	case DataFormat::R16G16B16A16_SInt:    return DXGI_FORMAT_R16G16B16A16_SINT;
	case DataFormat::R16G16B16A16_UNorm:   return DXGI_FORMAT_R16G16B16A16_UNORM;
	case DataFormat::R16G16B16A16_SNorm:   return DXGI_FORMAT_R16G16B16A16_SNORM;
	case DataFormat::R16G16B16A16_Float:   return DXGI_FORMAT_R16G16B16A16_FLOAT;

		// 四通道32位
	case DataFormat::R32G32B32A32_UInt:    return DXGI_FORMAT_R32G32B32A32_UINT;
	case DataFormat::R32G32B32A32_SInt:    return DXGI_FORMAT_R32G32B32A32_SINT;
	case DataFormat::R32G32B32A32_Float:   return DXGI_FORMAT_R32G32B32A32_FLOAT;

		// 深度/模板格式（严格匹配DX12的深度模板布局）
	case DataFormat::D16_UNorm:          return DXGI_FORMAT_D16_UNORM;
	case DataFormat::D24_UNorm_S8_UInt:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DataFormat::D32_Float:          return DXGI_FORMAT_D32_FLOAT;
	case DataFormat::D32_Float_S8_UInt:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT; // DX12中S8需搭配X24对齐

		// 压缩纹理格式（BC系列对应DXGI的压缩格式）
	case DataFormat::BC1_UNorm:         return DXGI_FORMAT_BC1_UNORM;
	case DataFormat::BC2_UNorm:         return DXGI_FORMAT_BC2_UNORM;
	case DataFormat::BC3_UNorm:         return DXGI_FORMAT_BC3_UNORM;
	case DataFormat::BC4_UNorm:         return DXGI_FORMAT_BC4_UNORM;
	case DataFormat::BC5_UNorm:         return DXGI_FORMAT_BC5_UNORM;
	case DataFormat::BC7_UNorm:         return DXGI_FORMAT_BC7_UNORM;

		// 未定义格式（错误情况，返回DXGI的未知格式）
	case DataFormat::Undefined:
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

// 将RAL图元拓扑转换为DX12图元拓扑
inline D3D_PRIMITIVE_TOPOLOGY ConvertToDX12PrimitiveTopology(RALPrimitiveTopologyType topology)
{
	switch (topology)
	{
	case RALPrimitiveTopologyType::PointList:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case RALPrimitiveTopologyType::LineList:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case RALPrimitiveTopologyType::LineStrip:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case RALPrimitiveTopologyType::TriangleList:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case RALPrimitiveTopologyType::TriangleStrip:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case RALPrimitiveTopologyType::LineListAdj:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
	case RALPrimitiveTopologyType::LineStripAdj:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
	case RALPrimitiveTopologyType::TriangleListAdj:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
	case RALPrimitiveTopologyType::TriangleStripAdj:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
	default:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; // 默认使用三角形列表
	}
}

// 将RAL资源状态转换为DX12资源状态
inline D3D12_RESOURCE_STATES ConvertToDX12ResourceState(RALResourceState state)
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

// DX12实现的Shader基类
class DX12RALShader : public IRALShader
{
public:
	DX12RALShader(RALShaderType shaderType)
		: IRALShader(shaderType)
	{
	}

	virtual ~DX12RALShader() = default;

	// 设置原生Shader指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生Shader指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或其他DX12 Shader相关指针
};

// DX12实现的顶点着色器
class DX12RALVertexShader : public IRALVertexShader
{
public:
	DX12RALVertexShader()
		: IRALVertexShader()
	{
	}

	virtual ~DX12RALVertexShader() = default;

	// 设置原生顶点着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生顶点着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的像素着色器
class DX12RALPixelShader : public IRALPixelShader
{
public:
	DX12RALPixelShader()
		: IRALPixelShader()
	{
	}

	virtual ~DX12RALPixelShader() = default;

	// 设置原生像素着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生像素着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的网格着色器
class DX12RALMeshShader : public IRALMeshShader
{
public:
	DX12RALMeshShader()
		: IRALMeshShader()
	{
	}

	virtual ~DX12RALMeshShader() = default;

	// 设置原生网格着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生网格着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的放大着色器
class DX12RALAmplificationShader : public IRALAmplificationShader
{
public:
	DX12RALAmplificationShader()
		: IRALAmplificationShader()
	{
	}

	virtual ~DX12RALAmplificationShader() = default;

	// 设置原生放大着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生放大着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的几何着色器
class DX12RALGeometryShader : public IRALGeometryShader
{
public:
	DX12RALGeometryShader()
		: IRALGeometryShader()
	{
	}

	virtual ~DX12RALGeometryShader() = default;

	// 设置原生几何着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生几何着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的计算着色器
class DX12RALComputeShader : public IRALComputeShader
{
public:
	DX12RALComputeShader()
		: IRALComputeShader()
	{
	}

	virtual ~DX12RALComputeShader() = default;

	// 设置原生计算着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生计算着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的光线生成着色器
class DX12RALRayGenShader : public IRALRayGenShader
{
public:
	DX12RALRayGenShader()
		: IRALRayGenShader()
	{
	}

	virtual ~DX12RALRayGenShader() = default;

	// 设置原生光线生成着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线生成着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的光线未命中着色器
class DX12RALRayMissShader : public IRALRayMissShader
{
public:
	DX12RALRayMissShader()
		: IRALRayMissShader()
	{
	}

	virtual ~DX12RALRayMissShader() = default;

	// 设置原生光线未命中着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线未命中着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的光线命中组着色器
class DX12RALRayHitGroupShader : public IRALRayHitGroupShader
{
public:
	DX12RALRayHitGroupShader()
		: IRALRayHitGroupShader()
	{
	}

	virtual ~DX12RALRayHitGroupShader() = default;

	// 设置原生光线命中组着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线命中组着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

	// 设置最近命中着色器
	void SetClosestHitShader(ID3DBlob* shader)
	{
		m_closestHitShader = shader;
	}

	// 设置相交着色器
	void SetIntersectionShader(ID3DBlob* shader)
	{
		m_intersectionShader = shader;
	}

	// 设置任意命中着色器
	void SetAnyHitShader(ID3DBlob* shader)
	{
		m_anyHitShader = shader;
	}

protected:
	void* m_nativeShader;        // 命中组相关指针
	ComPtr<ID3DBlob> m_closestHitShader;    // 最近命中着色器
	ComPtr<ID3DBlob> m_intersectionShader;  // 相交着色器
	ComPtr<ID3DBlob> m_anyHitShader;        // 任意命中着色器
};

// DX12实现的光线可调用着色器
class DX12RALRayCallableShader : public IRALRayCallableShader
{
public:
	DX12RALRayCallableShader()
		: IRALRayCallableShader()
	{
	}

	virtual ~DX12RALRayCallableShader() = default;

	// 设置原生光线可调用着色器指针
	void SetNativeShader(ID3DBlob* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线可调用着色器指针
	ID3DBlob* GetNativeShader() const
	{
		return m_nativeShader.Get();
	}

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return GetNativeShader();
	}

protected:
	ComPtr<ID3DBlob> m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的Texture
class DX12RALTexture : public IRALTexture
{
public:
	DX12RALTexture(ETextureType textureType)
		: IRALTexture(textureType)
	{
	}

	virtual ~DX12RALTexture() = default;

	// 设置原生资源指针
	void SetNativeResource(ID3D12Resource* nativeResource)
	{
		m_nativeResource.Attach(nativeResource);
	}

	// 获取原生资源指针
	void* GetNativeResource() const
	{
		return m_nativeResource.Get();
	}

	// 设置原生Shader resource view
	void SetNativeShaderResourceView(void* srv)
	{
		m_nativeShaderResourceView = srv;
	}

	// 获取原生Shader resource view
	virtual void* GetNativeShaderResourceView() const override
	{
		return m_nativeShaderResourceView;
	}

	// 设置原生Render target view
	void SetNativeRenderTargetView(void* rtv)
	{
		m_nativeRenderTargetView = rtv;
	}

	// 获取原生Render target view
	void* GetNativeRenderTargetView() const
	{
		return m_nativeRenderTargetView;
	}

	// 设置原生Depth stencil view
	void SetNativeDepthStencilView(void* dsv)
	{
		m_nativeDepthStencilView = dsv;
	}

	// 获取原生Depth stencil view
	void* GetNativeDepthStencilView() const
	{
		return m_nativeDepthStencilView;
	}

protected:
	ComPtr<ID3D12Resource> m_nativeResource;             // ID3D12Resource*
	void* m_nativeShaderResourceView;   // ID3D12DescriptorHeap中的SRV
	void* m_nativeRenderTargetView;     // ID3D12DescriptorHeap中的RTV
	void* m_nativeDepthStencilView;     // ID3D12DescriptorHeap中的DSV
};

// DX12实现的顶点缓冲区
class DX12RALVertexBuffer : public IRALVertexBuffer
{
public:
	DX12RALVertexBuffer(uint32_t size, uint32_t stride)
		: IRALVertexBuffer(size)
		, m_stride(stride)
	{
	}

	virtual ~DX12RALVertexBuffer() = default;

	// 设置原生资源指针
	void SetNativeResource(ID3D12Resource* resource)
	{
		m_nativeResource = resource;
	}

	// 获取原生资源指针
	virtual void* GetNativeResource() const override
	{
		return m_nativeResource.Get();
	}

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbView = {};

		if (m_nativeResource)
		{
			vbView.BufferLocation = m_nativeResource->GetGPUVirtualAddress();
			vbView.SizeInBytes = static_cast<UINT>(GetSize());
			vbView.StrideInBytes = m_stride;
		}
		return vbView;
	}

protected:
	ComPtr<ID3D12Resource> m_nativeResource;     // ID3D12Resource*
	uint32_t m_stride;                          // 顶点步长
};

// DX12实现的索引缓冲区
class DX12RALIndexBuffer : public IRALIndexBuffer
{
public:
	DX12RALIndexBuffer(uint64_t size, bool is32BitIndex)
		: IRALIndexBuffer(size, is32BitIndex)
	{
	}

	virtual ~DX12RALIndexBuffer() = default;

	// 设置原生资源指针
	void SetNativeResource(ID3D12Resource* resource)
	{
		m_nativeResource = resource;
	}

	// 获取原生资源指针
	virtual void* GetNativeResource() const override
	{
		return m_nativeResource.Get();
	}

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibView = {};
		if (m_nativeResource)
		{
			ibView.BufferLocation = m_nativeResource->GetGPUVirtualAddress();
			ibView.SizeInBytes = static_cast<UINT>(GetSize());
			ibView.Format = Is32BitIndex() ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		}
		return ibView;
	}
protected:
	ComPtr<ID3D12Resource> m_nativeResource;     // ID3D12Resource*
};

// DX12实现的常量缓冲区
class DX12RALConstBuffer : public IRALConstBuffer
{
public:
	DX12RALConstBuffer(uint32_t size)
		: IRALConstBuffer(size)
	{
	}

	virtual ~DX12RALConstBuffer() = default;

	// 获取原生资源指针
	virtual void* GetNativeResource() const override
	{
		return m_nativeResource.Get();
	}

	// 设置原生资源指针
	void SetNativeResource(ID3D12Resource* resource)
	{
		m_nativeResource = resource;
	}

	virtual bool Map(void** ppData) override
	{
		D3D12_RANGE range;
		range.Begin = 0;
		range.End = GetSize();

		HRESULT hr = m_nativeResource->Map(0, &range, ppData);

		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	virtual void Unmap() override
	{
		m_nativeResource->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress()
	{
		return m_nativeResource->GetGPUVirtualAddress();
	}

protected:
	ComPtr<ID3D12Resource> m_nativeResource;     // ID3D12Resource*
};

// DX12实现的渲染目标
class DX12RALRenderTarget : public IRALRenderTarget
{
public:
	DX12RALRenderTarget(uint32_t width, uint32_t height, DataFormat format)
		: IRALRenderTarget(width, height, format)
	{
	}

	virtual ~DX12RALRenderTarget() = default;

	// 获取原生资源指针
	void* GetNativeResource() const
	{
		return m_nativeResource.Get();
	}

	// 设置原生资源指针
	void SetNativeResource(ID3D12Resource* resource)
	{
		m_nativeResource = resource;
	}

	// 设置原生渲染目标视图
	void SetNativeRenderTargetView(void* rtv)
	{
		m_nativeRenderTargetView = rtv;
	}

protected:
	ComPtr<ID3D12Resource> m_nativeResource;           // ID3D12Resource*
	void* m_nativeRenderTargetView;   // ID3D12DescriptorHeap中的RTV
};

// DX12实现的DepthStencil
class DX12RALDepthStencil : public IRALDepthStencil
{
public:
	DX12RALDepthStencil(uint32_t width, uint32_t height, DataFormat format)
		: IRALDepthStencil(width, height, format)
	{
	}

	virtual ~DX12RALDepthStencil() = default;

	// 获取原生资源指针
	void* GetNativeResource() const
	{
		return m_nativeResource.Get();
	}

	// 设置原生资源指针
	void SetNativeResource(ID3D12Resource* resource)
	{
		m_nativeResource = resource;
	}

	// 设置原生渲染目标视图
	void SetNativeRenderTargetView(void* rtv)
	{
		m_nativeRenderTargetView = rtv;
	}

protected:
	ComPtr<ID3D12Resource> m_nativeResource;           // ID3D12Resource*
	void* m_nativeRenderTargetView;   // ID3D12DescriptorHeap中的DTV
};

// DX12实现的根签名
class DX12RALRootSignature : public IRALRootSignature
{
public:
	DX12RALRootSignature()
		: IRALRootSignature()
	{
	}

	virtual ~DX12RALRootSignature() = default;

	// 设置原生根签名指针
	void SetNativeRootSignature(ID3D12RootSignature* rootSignature)
	{
		m_nativeRootSignature = rootSignature;
	}

	// 获取原生根签名指针
	virtual void* GetNativeResource() const override
	{
		return m_nativeRootSignature.Get();
	}

protected:
	ComPtr<ID3D12RootSignature> m_nativeRootSignature; // ID3D12RootSignature*
};

// DX12实现的图形管线状态
class DX12RALGraphicsPipelineState : public IRALGraphicsPipelineState
{
public:
	DX12RALGraphicsPipelineState()
		: IRALGraphicsPipelineState()
	{
	}

	virtual ~DX12RALGraphicsPipelineState() = default;

	// 设置原生管线状态指针
	void SetNativePipelineState(ID3D12PipelineState* pipelineState)
	{
		m_nativePipelineState = pipelineState;
	}

	// 获取原生管线状态指针
	virtual void* GetNativeResource() const override
	{
		return m_nativePipelineState.Get();
	}

protected:
	ComPtr<ID3D12PipelineState> m_nativePipelineState; // ID3D12PipelineState*
};

#endif // DX12RALRESOURCE_H