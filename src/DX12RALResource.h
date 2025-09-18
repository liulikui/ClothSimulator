#ifndef DX12RALRESOURCE_H
#define DX12RALRESOURCE_H

#include "RALResource.h"
#include <dxgiformat.h>

// 将跨平台DataFormat转换为DX12的DXGI_FORMAT
DXGI_FORMAT toDXGIFormat(DataFormat format) {
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

// DX12实现的RAL Resource基类
class DX12RALResource : public IRALResource
{
public:
	DX12RALResource(RALResourceType type)
		: IRALResource(type)
	{
	}

	virtual ~DX12RALResource() = default;
};

// DX12实现的Shader基类
class DX12RALShader : public IRALShader
{
public:
	DX12RALShader(RALShaderType shaderType)
		: IRALShader(shaderType)
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALShader() = default;

	// 设置原生Shader指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生Shader指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或其他DX12 Shader相关指针
};

// DX12实现的顶点着色器
class DX12RALVertexShader : public IRALVertexShader
{
public:
	DX12RALVertexShader()
		: IRALVertexShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALVertexShader() = default;

	// 设置原生顶点着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生顶点着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的像素着色器
class DX12RALPixelShader : public IRALPixelShader
{
public:
	DX12RALPixelShader()
		: IRALPixelShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALPixelShader() = default;

	// 设置原生像素着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生像素着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的网格着色器
class DX12RALMeshShader : public IRALMeshShader
{
public:
	DX12RALMeshShader()
		: IRALMeshShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALMeshShader() = default;

	// 设置原生网格着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生网格着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的放大着色器
class DX12RALAmplificationShader : public IRALAmplificationShader
{
public:
	DX12RALAmplificationShader()
		: IRALAmplificationShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALAmplificationShader() = default;

	// 设置原生放大着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生放大着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的几何着色器
class DX12RALGeometryShader : public IRALGeometryShader
{
public:
	DX12RALGeometryShader()
		: IRALGeometryShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALGeometryShader() = default;

	// 设置原生几何着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生几何着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的计算着色器
class DX12RALComputeShader : public IRALComputeShader
{
public:
	DX12RALComputeShader()
		: IRALComputeShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALComputeShader() = default;

	// 设置原生计算着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生计算着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的光线生成着色器
class DX12RALRayGenShader : public IRALRayGenShader
{
public:
	DX12RALRayGenShader()
		: IRALRayGenShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALRayGenShader() = default;

	// 设置原生光线生成着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线生成着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的光线未命中着色器
class DX12RALRayMissShader : public IRALRayMissShader
{
public:
	DX12RALRayMissShader()
		: IRALRayMissShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALRayMissShader() = default;

	// 设置原生光线未命中着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线未命中着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的光线命中组着色器
class DX12RALRayHitGroupShader : public IRALRayHitGroupShader
{
public:
	DX12RALRayHitGroupShader()
		: IRALRayHitGroupShader()
		, m_nativeShader(nullptr)
		, m_closestHitShader(nullptr)
		, m_intersectionShader(nullptr)
		, m_anyHitShader(nullptr)
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

	// 设置最近命中着色器
	void SetClosestHitShader(void* shader)
	{
		m_closestHitShader = shader;
	}

	// 设置相交着色器
	void SetIntersectionShader(void* shader)
	{
		m_intersectionShader = shader;
	}

	// 设置任意命中着色器
	void SetAnyHitShader(void* shader)
	{
		m_anyHitShader = shader;
	}

protected:
	void* m_nativeShader;        // 命中组相关指针
	void* m_closestHitShader;    // 最近命中着色器
	void* m_intersectionShader;  // 相交着色器
	void* m_anyHitShader;        // 任意命中着色器
};

// DX12实现的光线可调用着色器
class DX12RALRayCallableShader : public IRALRayCallableShader
{
public:
	DX12RALRayCallableShader()
		: IRALRayCallableShader()
		, m_nativeShader(nullptr)
	{
	}

	virtual ~DX12RALRayCallableShader() = default;

	// 设置原生光线可调用着色器指针
	void SetNativeShader(void* shader)
	{
		m_nativeShader = shader;
	}

	// 获取原生光线可调用着色器指针
	void* GetNativeShader() const
	{
		return m_nativeShader;
	}

protected:
	void* m_nativeShader; // ID3D12ShaderBytecode* 或 ID3DBlob*
};

// DX12实现的Viewable资源
class DX12RALViewableResource : public IRALViewableResource
{
public:
	DX12RALViewableResource(RALResourceType type)
		: IRALViewableResource(type)
	{
	}

	virtual ~DX12RALViewableResource() = default;
};

// DX12实现的Texture
class DX12RALTexture : public IRALTexture
{
public:
	DX12RALTexture(ETextureType textureType)
		: IRALTexture(textureType)
		, m_nativeResource(nullptr)
		, m_nativeShaderResourceView(nullptr)
		, m_nativeRenderTargetView(nullptr)
		, m_nativeDepthStencilView(nullptr)
	{
	}

	virtual ~DX12RALTexture() = default;

	// 设置原生资源指针
	void SetNativeResource(void* resource)
	{
		m_nativeResource = resource;
	}

	// 获取原生资源指针
	virtual void* GetNativeResource() const override
	{
		return m_nativeResource;
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
	void* m_nativeResource;             // ID3D12Resource*
	void* m_nativeShaderResourceView;   // ID3D12DescriptorHeap中的SRV
	void* m_nativeRenderTargetView;     // ID3D12DescriptorHeap中的RTV
	void* m_nativeDepthStencilView;     // ID3D12DescriptorHeap中的DSV
};

// DX12实现的顶点缓冲区视图
class DX12VertexBufferView : public IVertexBufferView
{
public:
	DX12VertexBufferView()
		: m_bufferLocation(0)
		, m_sizeInBytes(0)
		, m_strideInBytes(0)
		, m_nativeView(nullptr)
	{
	}

	virtual ~DX12VertexBufferView() = default;

	// 设置缓冲区起始地址
	void SetBufferLocation(uint64_t location)
	{
		m_bufferLocation = location;
	}

	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const override
	{
		return m_bufferLocation;
	}

	// 设置缓冲区大小
	void SetSizeInBytes(uint32_t size)
	{
		m_sizeInBytes = size;
	}

	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const override
	{
		return m_sizeInBytes;
	}

	// 设置顶点步长
	void SetStrideInBytes(uint32_t stride)
	{
		m_strideInBytes = stride;
	}

	// 获取顶点步长
	virtual uint32_t GetStrideInBytes() const override
	{
		return m_strideInBytes;
	}

	// 设置原生视图指针
	void SetNativeView(void* view)
	{
		m_nativeView = view;
	}

	// 获取原生视图指针
	virtual void* GetNativeView() override
	{
		return m_nativeView;
	}

protected:
	uint64_t m_bufferLocation;  // 缓冲区起始地址
	uint32_t m_sizeInBytes;     // 缓冲区大小
	uint32_t m_strideInBytes;   // 顶点步长
	void* m_nativeView;         // D3D12_VERTEX_BUFFER_VIEW*
};

// DX12实现的索引缓冲区视图
class DX12IndexBufferView : public IIndexBufferView
{
public:
	DX12IndexBufferView()
		: m_bufferLocation(0)
		, m_sizeInBytes(0)
		, m_is32Bit(false)
		, m_nativeView(nullptr)
	{
	}

	virtual ~DX12IndexBufferView() = default;

	// 设置缓冲区起始地址
	void SetBufferLocation(uint64_t location)
	{
		m_bufferLocation = location;
	}

	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const override
	{
		return m_bufferLocation;
	}

	// 设置缓冲区大小
	void SetSizeInBytes(uint32_t size)
	{
		m_sizeInBytes = size;
	}

	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const override
	{
		return m_sizeInBytes;
	}

	// 设置索引格式（32位或16位）
	void SetIs32Bit(bool is32Bit)
	{
		m_is32Bit = is32Bit;
	}

	// 获取索引格式（32位或16位）
	virtual bool Is32Bit() const override
	{
		return m_is32Bit;
	}

	// 设置原生视图指针
	void SetNativeView(void* view)
	{
		m_nativeView = view;
	}

	// 获取原生视图指针
	virtual void* GetNativeView() override
	{
		return m_nativeView;
	}

protected:
	uint64_t m_bufferLocation;  // 缓冲区起始地址
	uint32_t m_sizeInBytes;     // 缓冲区大小
	bool m_is32Bit;             // 是否为32位索引
	void* m_nativeView;         // D3D12_INDEX_BUFFER_VIEW*
};

// DX12实现的顶点缓冲区
class DX12RALVertexBuffer : public IVertexBuffer
{
public:
	DX12RALVertexBuffer()
		: m_nativeResource(nullptr)
		, m_totalSizeInBytes(0)
	{
	}

	virtual ~DX12RALVertexBuffer() = default;

	// 创建顶点缓冲区视图
	virtual IVertexBufferView* CreateVertexBufferView(uint32_t stride, uint32_t sizeInBytes) override;

	// 设置原生资源指针
	void SetNativeResource(void* resource)
	{
		m_nativeResource = resource;
	}

	// 获取原生资源指针
	void* GetNativeResource() const
	{
		return m_nativeResource;
	}

	// 设置总大小
	void SetTotalSizeInBytes(uint32_t size)
	{
		m_totalSizeInBytes = size;
	}

	// 获取总大小
	uint32_t GetTotalSizeInBytes() const
	{
		return m_totalSizeInBytes;
	}

protected:
	void* m_nativeResource;     // ID3D12Resource*
	uint32_t m_totalSizeInBytes; // 缓冲区总大小
};

// DX12实现的索引缓冲区
class DX12RALIndexBuffer : public IIndexBuffer
{
public:
	DX12RALIndexBuffer()
		: m_nativeResource(nullptr)
		, m_totalSizeInBytes(0)
		, m_is32BitIndex(false)
	{
	}

	virtual ~DX12RALIndexBuffer() = default;

	// 创建索引缓冲区视图
	virtual IIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytes, bool is32Bit) override;

	// 设置原生资源指针
	void SetNativeResource(void* resource)
	{
		m_nativeResource = resource;
	}

	// 获取原生资源指针
	void* GetNativeResource() const
	{
		return m_nativeResource;
	}

	// 设置总大小
	void SetTotalSizeInBytes(uint32_t size)
	{
		m_totalSizeInBytes = size;
	}

	// 获取总大小
	uint32_t GetTotalSizeInBytes() const
	{
		return m_totalSizeInBytes;
	}

	// 设置索引格式
	void SetIs32BitIndex(bool is32Bit)
	{
		m_is32BitIndex = is32Bit;
	}

	// 获取索引格式
	bool Is32BitIndex() const
	{
		return m_is32BitIndex;
	}

protected:
	void* m_nativeResource;     // ID3D12Resource*
	uint32_t m_totalSizeInBytes; // 缓冲区总大小
	bool m_is32BitIndex;        // 是否为32位索引
};

// DX12实现的常量缓冲区视图
class DX12ConstBufferView : public IConstBufferView
{
public:
	DX12ConstBufferView()
		: m_bufferLocation(0)
		, m_sizeInBytes(0)
		, m_nativeView(nullptr)
	{
	}

	virtual ~DX12ConstBufferView() = default;

	// 设置缓冲区起始地址
	void SetBufferLocation(uint64_t location)
	{
		m_bufferLocation = location;
	}

	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const override
	{
		return m_bufferLocation;
	}

	// 设置缓冲区大小
	void SetSizeInBytes(uint32_t size)
	{
		m_sizeInBytes = size;
	}

	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const override
	{
		return m_sizeInBytes;
	}

	// 设置原生视图指针
	void SetNativeView(void* view)
	{
		m_nativeView = view;
	}

	// 获取原生视图指针
	virtual void* GetNativeView() override
	{
		return m_nativeView;
	}

protected:
	uint64_t m_bufferLocation;  // 缓冲区起始地址
	uint32_t m_sizeInBytes;     // 缓冲区大小
	void* m_nativeView;         // D3D12_CONSTANT_BUFFER_VIEW_DESC*
};

// DX12实现的常量缓冲区
class DX12RALConstBuffer : public IConstBuffer
{
public:
	DX12RALConstBuffer()
		: m_nativeResource(nullptr)
		, m_totalSizeInBytes(0)
	{
	}

	virtual ~DX12RALConstBuffer() = default;

	// 创建常量缓冲区视图
	virtual IConstBufferView* CreateConstBufferView(uint32_t sizeInBytes) override;

	// 更新常量缓冲区数据
	virtual void Update(const void* data, uint32_t sizeInBytes) override;

	// 设置原生资源指针
	void SetNativeResource(void* resource)
	{
		m_nativeResource = resource;
	}

	// 获取原生资源指针
	void* GetNativeResource() const
	{
		return m_nativeResource;
	}

	// 设置总大小
	void SetTotalSizeInBytes(uint32_t size)
	{
		m_totalSizeInBytes = size;
	}

	// 获取总大小
	uint32_t GetTotalSizeInBytes() const
	{
		return m_totalSizeInBytes;
	}

protected:
	void* m_nativeResource;     // ID3D12Resource*
	uint32_t m_totalSizeInBytes; // 缓冲区总大小
};

// DX12实现的渲染目标
class DX12RALRenderTarget : public IRALRenderTarget
{
public:
	DX12RALRenderTarget()
		: m_nativeResource(nullptr)
		, m_nativeRenderTargetView(nullptr)
		, m_width(0)
		, m_height(0)
		, m_format(DataFormat::Undefined)
	{
	}

	virtual ~DX12RALRenderTarget() = default;

	// 获取渲染目标宽度
	virtual uint32_t GetWidth() const override
	{
		return m_width;
	}

	// 获取渲染目标高度
	virtual uint32_t GetHeight() const override
	{
		return m_height;
	}

	// 获取渲染目标格式
	virtual DataFormat GetFormat() const override
	{
		return m_format;
	}

	// 清除渲染目标
	virtual void Clear(const float color[4]) override
	{
		// 实现将在DX12Renderer中完成
	}

	// 获取原生渲染目标视图
	virtual void* GetNativeRenderTargetView() const override
	{
		return m_nativeRenderTargetView;
	}

	// 获取原生资源指针
	virtual void* GetNativeResource() const override
	{
		return m_nativeResource;
	}

	// 设置原生资源指针
	void SetNativeResource(void* resource)
	{
		m_nativeResource = resource;
	}

	// 设置原生渲染目标视图
	void SetNativeRenderTargetView(void* rtv)
	{
		m_nativeRenderTargetView = rtv;
	}

	// 设置宽度
	void SetWidth(uint32_t width)
	{
		m_width = width;
	}

	// 设置高度
	void SetHeight(uint32_t height)
	{
		m_height = height;
	}

	// 设置格式
	void SetFormat(DataFormat format)
	{
		m_format = format;
	}

protected:
	void* m_nativeResource;           // ID3D12Resource*
	void* m_nativeRenderTargetView;   // ID3D12DescriptorHeap中的RTV
	uint32_t m_width;                 // 渲染目标宽度
	uint32_t m_height;                // 渲染目标高度
	DataFormat m_format;              // 渲染目标格式
};

#endif // DX12RALRESOURCE_H