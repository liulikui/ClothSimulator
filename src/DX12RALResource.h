#ifndef DX12RALRESOURCE_H
#define DX12RALRESOURCE_H

#include "RALResource.h"
#include "TRefCountPtr.h"
#include <dxgiformat.h>
#include <d3dcommon.h>
#include <d3d12.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// 前置声明
class DX12RALDevice;

// 将跨平台RALDataFormat转换为DX12的DXGI_FORMAT
inline DXGI_FORMAT toDXGIFormat(RALDataFormat format) 
{
	switch (format) 
	{
		// 单通道8位
	case RALDataFormat::R8_UInt:          return DXGI_FORMAT_R8_UINT;
	case RALDataFormat::R8_SInt:          return DXGI_FORMAT_R8_SINT;
	case RALDataFormat::R8_UNorm:         return DXGI_FORMAT_R8_UNORM;
	case RALDataFormat::R8_SNorm:         return DXGI_FORMAT_R8_SNORM;

		// 单通道16位
	case RALDataFormat::R16_UInt:         return DXGI_FORMAT_R16_UINT;
	case RALDataFormat::R16_SInt:         return DXGI_FORMAT_R16_SINT;
	case RALDataFormat::R16_UNorm:        return DXGI_FORMAT_R16_UNORM;
	case RALDataFormat::R16_SNorm:        return DXGI_FORMAT_R16_SNORM;
	case RALDataFormat::R16_Float:        return DXGI_FORMAT_R16_FLOAT;

		// 单通道32位
	case RALDataFormat::R32_UInt:         return DXGI_FORMAT_R32_UINT;
	case RALDataFormat::R32_SInt:         return DXGI_FORMAT_R32_SINT;
	case RALDataFormat::R32_Float:        return DXGI_FORMAT_R32_FLOAT;
	case RALDataFormat::R32_Typeless:     return DXGI_FORMAT_R32_TYPELESS; // 对应DXGI_FORMAT_R32_TYPELESS

		// 单通道16位无类型
	case RALDataFormat::R16_Typeless:     return DXGI_FORMAT_R16_TYPELESS;

		// 单通道8位无类型
	case RALDataFormat::R8_Typeless:      return DXGI_FORMAT_R8_TYPELESS;

		// 双通道8位
	case RALDataFormat::R8G8_UInt:        return DXGI_FORMAT_R8G8_UINT;
	case RALDataFormat::R8G8_SInt:        return DXGI_FORMAT_R8G8_SINT;
	case RALDataFormat::R8G8_UNorm:       return DXGI_FORMAT_R8G8_UNORM;
	case RALDataFormat::R8G8_SNorm:       return DXGI_FORMAT_R8G8_SNORM;

		// 双通道16位
	case RALDataFormat::R16G16_UInt:      return DXGI_FORMAT_R16G16_UINT;
	case RALDataFormat::R16G16_SInt:      return DXGI_FORMAT_R16G16_SINT;
	case RALDataFormat::R16G16_UNorm:     return DXGI_FORMAT_R16G16_UNORM;
	case RALDataFormat::R16G16_SNorm:     return DXGI_FORMAT_R16G16_SNORM;
	case RALDataFormat::R16G16_Float:     return DXGI_FORMAT_R16G16_FLOAT;
	case RALDataFormat::R16G16_Typeless:  return DXGI_FORMAT_R16G16_TYPELESS;

		// 双通道8位无类型
	case RALDataFormat::R8G8_Typeless:    return DXGI_FORMAT_R8G8_TYPELESS;

		// 双通道32位
	case RALDataFormat::R32G32_UInt:      return DXGI_FORMAT_R32G32_UINT;
	case RALDataFormat::R32G32_SInt:      return DXGI_FORMAT_R32G32_SINT;
	case RALDataFormat::R32G32_Float:     return DXGI_FORMAT_R32G32_FLOAT;
	case RALDataFormat::R32G32_Typeless:  return DXGI_FORMAT_R32G32_TYPELESS;

		// 三通道8位格式（DXGI不原生支持，返回UNKNOWN）
	case RALDataFormat::R8G8B8_UInt:
	case RALDataFormat::R8G8B8_SInt:
	case RALDataFormat::R8G8B8_UNorm:
	case RALDataFormat::R8G8B8_SNorm:
		// 注意：DXGI不支持纯R8G8B8格式（3字节未对齐），建议使用R8G8B8A8替代
		return DXGI_FORMAT_UNKNOWN;

	   // 三通道32位
	case RALDataFormat::R32G32B32_Float:  return DXGI_FORMAT_R32G32B32_FLOAT;
	case RALDataFormat::R32G32B32_Typeless: return DXGI_FORMAT_R32G32B32_TYPELESS;

		// 四通道8位
	case RALDataFormat::R8G8B8A8_UInt:    return DXGI_FORMAT_R8G8B8A8_UINT;
	case RALDataFormat::R8G8B8A8_SInt:    return DXGI_FORMAT_R8G8B8A8_SINT;
	case RALDataFormat::R8G8B8A8_UNorm:   return DXGI_FORMAT_R8G8B8A8_UNORM;
	case RALDataFormat::R8G8B8A8_SNorm:   return DXGI_FORMAT_R8G8B8A8_SNORM;
	case RALDataFormat::R8G8B8A8_SRGB:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // SRGB对应DXGI的UNORM_SRGB
	case RALDataFormat::R8G8B8A8_Typeless: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	// 注意：DXGI中没有R8G8B8X8_TYPELESS和B8G8R8X8_TYPELESS，使用对应的A8版本代替
	case RALDataFormat::R8G8B8X8_Typeless: return DXGI_FORMAT_R8G8B8A8_TYPELESS; // 使用R8G8B8A8_TYPELESS代替
	case RALDataFormat::B8G8R8A8_UNorm:   return DXGI_FORMAT_B8G8R8A8_UNORM;
	case RALDataFormat::B8G8R8A8_SRGB:    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	case RALDataFormat::B8G8R8A8_Typeless: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	case RALDataFormat::B8G8R8X8_Typeless: return DXGI_FORMAT_B8G8R8A8_TYPELESS; // 使用B8G8R8A8_TYPELESS代替

		// 四通道16位
	case RALDataFormat::R16G16B16A16_UInt:    return DXGI_FORMAT_R16G16B16A16_UINT;
	case RALDataFormat::R16G16B16A16_SInt:    return DXGI_FORMAT_R16G16B16A16_SINT;
	case RALDataFormat::R16G16B16A16_UNorm:   return DXGI_FORMAT_R16G16B16A16_UNORM;
	case RALDataFormat::R16G16B16A16_SNorm:   return DXGI_FORMAT_R16G16B16A16_SNORM;
	case RALDataFormat::R16G16B16A16_Float:   return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case RALDataFormat::R16G16B16A16_Typeless: return DXGI_FORMAT_R16G16B16A16_TYPELESS;

		// 四通道32位
	case RALDataFormat::R32G32B32A32_UInt:    return DXGI_FORMAT_R32G32B32A32_UINT;
	case RALDataFormat::R32G32B32A32_SInt:    return DXGI_FORMAT_R32G32B32A32_SINT;
	case RALDataFormat::R32G32B32A32_Float:   return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case RALDataFormat::R32G32B32A32_Typeless: return DXGI_FORMAT_R32G32B32A32_TYPELESS;

		// 深度/模板格式（严格匹配DX12的深度模板布局）
	case RALDataFormat::D16_UNorm:          return DXGI_FORMAT_D16_UNORM;
	case RALDataFormat::D24_UNorm_S8_UInt:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case RALDataFormat::D32_Float:          return DXGI_FORMAT_D32_FLOAT;
	case RALDataFormat::D32_Float_S8_UInt:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT; // DX12中S8需搭配X24对齐
	case RALDataFormat::D24_UNorm_S8_UInt_Typeless: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case RALDataFormat::D32_Float_S8_UInt_Typeless: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		// 压缩纹理格式（BC系列对应DXGI的压缩格式）
	case RALDataFormat::BC1_UNorm:         return DXGI_FORMAT_BC1_UNORM;
	case RALDataFormat::BC2_UNorm:         return DXGI_FORMAT_BC2_UNORM;
	case RALDataFormat::BC3_UNorm:         return DXGI_FORMAT_BC3_UNORM;
	case RALDataFormat::BC4_UNorm:         return DXGI_FORMAT_BC4_UNORM;
	case RALDataFormat::BC5_UNorm:         return DXGI_FORMAT_BC5_UNORM;
	case RALDataFormat::BC7_UNorm:         return DXGI_FORMAT_BC7_UNORM;

		// 未定义格式（错误情况，返回DXGI的未知格式）
	case RALDataFormat::Undefined:
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
static D3D12_RESOURCE_STATES ConvertToDX12ResourceState(RALResourceState state)
{
	switch (state)
	{
	case RALResourceState::Common:
		return D3D12_RESOURCE_STATE_COMMON;
	case RALResourceState::CopySource:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case RALResourceState::CopyDest:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	case RALResourceState::VertexBuffer:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case RALResourceState::IndexBuffer:
		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	case RALResourceState::ShaderResource:
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case RALResourceState::RenderTarget:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case RALResourceState::DepthStencil:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
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
	DX12RALIndexBuffer(uint32_t count, uint32_t size, bool is32BitIndex)
		: IRALIndexBuffer(count, size, is32BitIndex)
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
	DX12RALRenderTarget(uint32_t width, uint32_t height, RALDataFormat format)
		: IRALRenderTarget(width, height, format)
	{
	}

	virtual ~DX12RALRenderTarget() override = default;

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

protected:
	ComPtr<ID3D12Resource> m_nativeResource;        // ID3D12Resource*
};

// DX12实现的DepthStencil
class DX12RALDepthStencil : public IRALDepthStencil
{
public:
	DX12RALDepthStencil(uint32_t width, uint32_t height, RALDataFormat format)
		: IRALDepthStencil(width, height, format)
	{
	}

	virtual ~DX12RALDepthStencil() override = default;

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

protected:
	ComPtr<ID3D12Resource> m_nativeResource;		// ID3D12Resource*
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

// DX12实现的深度模板视图
class DX12RALDepthStencilView : public IRALDepthStencilView
{
public:
	DX12RALDepthStencilView()
		: IRALDepthStencilView()
		, m_device(nullptr)
		, m_dsvIndex(0)
	{
		m_dsvHandle.ptr = 0;
	}

	virtual ~DX12RALDepthStencilView() override;

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return const_cast<void*>(reinterpret_cast<const void*>(&m_dsvHandle));
	}

	// 实现IRALDepthStencilView接口
	virtual IRALDepthStencil* GetDepthStencil() const override
	{
		return m_depthStencil.Get();
	}

	virtual void* GetNativeDepthStencilView() const override
	{
		return const_cast<void*>(reinterpret_cast<const void*>(&m_dsvHandle));
	}

	// 设置深度模板资源
	void SetDepthStencil(IRALDepthStencil* depthStencil)
	{
		m_depthStencil = depthStencil;
	}

	// 设置DSV描述符句柄
	void SetDSVHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		m_dsvHandle = handle;
	}

	// 设置设备指针
	void SetDevice(DX12RALDevice* device)
	{
		m_device = device;
	}

	// 设置DSV索引
	void SetDSVIndex(uint32_t index)
	{
		m_dsvIndex = index;
	}

	// 设置DSV堆
	void SetDSVHeap(ID3D12DescriptorHeap* heap)
	{
		m_dsvHeap = heap;
	}

	// 获取DSV描述符句柄
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const
	{
		return m_dsvHandle;
	}

	// 获取DSV堆
	ID3D12DescriptorHeap* GetDSVHeap() const
	{
		return m_dsvHeap.Get();
	}

protected:
	TRefCountPtr<IRALDepthStencil> m_depthStencil;	// 关联的深度模板资源
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;		// DSV描述符句柄
	DX12RALDevice* m_device;						// 设备指针
	uint32_t m_dsvIndex;							// DSV索引
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;			// DSV堆
};

// DX12实现的渲染目标视图
class DX12RALRenderTargetView : public IRALRenderTargetView
{
public:
	DX12RALRenderTargetView()
		: IRALRenderTargetView()
		, m_device(nullptr)
		, m_rtvIndex(0)
	{
		m_rtvHandle.ptr = 0;
	}

	virtual ~DX12RALRenderTargetView() override;

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return const_cast<void*>(reinterpret_cast<const void*>(&m_rtvHandle));
	}

	// 实现IRALRenderTargetView接口
	virtual IRALRenderTarget* GetRenderTarget() const override
	{
		return m_renderTarget.Get();
	}

	virtual void* GetNativeRenderTargetView() const override
	{
		return const_cast<void*>(reinterpret_cast<const void*>(&m_rtvHandle));
	}

	// 设置渲染目标资源
	void SetRenderTarget(IRALRenderTarget* renderTarget)
	{
		m_renderTarget = renderTarget;
	}

	// 设置RTV描述符句柄
	void SetRTVHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		m_rtvHandle = handle;
	}

	// 获取RTV堆
	ID3D12DescriptorHeap* GetRTVHeap() const
	{
		return m_rtvHeap.Get();
	}

	// 获取RTV描述符句柄
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const
	{
		return m_rtvHandle;
	}

	// 设置RTV索引
	void SetRTVIndex(uint32_t index)
	{
		m_rtvIndex = index;
	}

	// 设置设备指针
	void SetDevice(DX12RALDevice* device)
	{
		m_device = device;
	}

	// 设置RTV堆
	void SetRTVHeap(ID3D12DescriptorHeap* rtvHeap)
	{
		m_rtvHeap = rtvHeap;
	}

protected:
	TRefCountPtr<IRALRenderTarget> m_renderTarget;		// 关联的渲染目标资源
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;			// RTV描述符句柄
	DX12RALDevice* m_device;							// 设备指针
	uint32_t m_rtvIndex;								// RTV索引
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;				// RTV堆
};

// DX12实现的着色器资源视图
class DX12RALShaderResourceView : public IRALShaderResourceView
{
public:
	DX12RALShaderResourceView()
		: IRALShaderResourceView()
		, m_device(nullptr)
		, m_srvIndex(0)
	{
		m_srvHandle.ptr = 0;
	}

	virtual ~DX12RALShaderResourceView() override;

	// 实现IRALResource接口
	virtual void* GetNativeResource() const override
	{
		return const_cast<void*>(reinterpret_cast<const void*>(&m_srvHandle));
	}

	// 实现IRALShaderResourceView接口
	virtual IRALResource* GetResource() const override
	{
		return m_resource.Get();
	}

	virtual void* GetNativeShaderResourceView() const override
	{
		return const_cast<void*>(reinterpret_cast<const void*>(&m_srvHandle));
	}

	// 设置关联的资源
	void SetResource(IRALResource* resource)
	{
		m_resource = resource;
	}

	// 设置SRV描述符句柄
	void SetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		m_srvHandle = handle;
	}

	// 获取SRV描述符句柄
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const
	{
		return m_srvHandle;
	}

	// 设置SRV堆
	void SetSRVHeap(ID3D12DescriptorHeap* srvHeap)
	{
		m_srvHeap = srvHeap;
	}

	// 获取SRV堆
	ID3D12DescriptorHeap* GetSRVHeap() const
	{
		return m_srvHeap.Get();
	}

	// 设置设备指针
	void SetDevice(DX12RALDevice* device)
	{
		m_device = device;
	}

	// 设置SRV索引
	void SetSRVIndex(uint32_t index)
	{
		m_srvIndex = index;
	}

protected:
	TRefCountPtr<IRALResource> m_resource;				// 关联的资源
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;            // SRV描述符句柄
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;             // SRV堆
	DX12RALDevice* m_device;                            // 设备指针
	uint32_t m_srvIndex;                                // SRV索引
};

#endif // DX12RALRESOURCE_H