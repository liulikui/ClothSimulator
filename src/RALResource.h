#ifndef RALRESOURCE_H
#define RALRESOURCE_H

#include <cstdint>
#include <atomic>
#include "DataFormat.h"

// Render Abstraction Layer资源类型
enum class RALResourceType
{
	NONE,
	UniformBuffer,
	UniformBufferLayout,
	Shader,
	GraphicsPipelineState,
	ComputePipelineState,
	RayTracingPipelineState,
	Texture,
	Viewport,

	Count,
};

// Render Abstraction Layer Shader类型
enum class RALShaderType
{
	Vertex,
	Pixel,
	Mesh,
	Amplification,
	Geometry,
	Compute,
	RayGen,
	RayMiss,
	RayHitGroup,
	RayCallable,

	Count,
};

// 顶点数据格式（描述单个属性的类型和长度）
enum class RALVertexFormat
{
	Float1,   // float
	Float2,   // float2
	Float3,   // float3
	Float4,   // float4
	Half2,    // half2 (16位浮点数)
	Half4,    // half4
	Int1,     // int32
	Int2,     // int32x2
	Int3,     // int32x3
	Int4,     // int32x4
	Uint1,    // uint32
	Uint2,    // uint32x2
	Uint3,    // uint32x3
	Uint4,    // uint32x4
	Byte4,    // int8x4 (通常用于压缩法线)
	UByte4,   // uint8x4 (通常用于颜色)
	Short2,   // int16x2
	Short4,   // int16x4
	UByte4N,  // uint8x4 归一化到 [0,1]
	Byte4N,   // int8x4 归一化到 [-1,1]
	Short2N,  // int16x2 归一化到 [-1,1]
	Short4N   // int16x4 归一化到 [-1,1]
};

// 顶点属性语义（用于标识属性用途，辅助上层逻辑）
enum class RALVertexSemantic
{
	Position,    // 顶点位置
	Normal,      // 法线
	Tangent,     // 切线
	Bitangent,   // 副切线
	TexCoord0,   // 纹理坐标集0
	TexCoord1,   // 纹理坐标集1
	Color0,      // 颜色集0
	Color1,      // 颜色集1
	BoneIndices, // 骨骼索引
	BoneWeights  // 骨骼权重
	// 可扩展更多语义
};

// 单个顶点属性的描述
struct RALVertexAttribute 
{
	RALVertexSemantic semantic;  // 属性语义（如POSITION）
	RALVertexFormat format;      // 数据格式（如Float3）
	uint32_t bufferSlot;      // 绑定的顶点缓冲区索引（支持多缓冲区）
	uint32_t offset;          // 在缓冲区中的字节偏移量
};

// 顶点缓冲区绑定描述（每个缓冲区单独配置）
struct RALVertexBufferBinding
{
	uint32_t stride;          // 每个顶点的字节步长（单个顶点数据总大小）
	bool isInstanceData;      // 是否为实例化数据（false=顶点数据，true=实例数据）
	uint32_t instanceStepRate;// 实例步长（每N个实例更新一次，仅isInstanceData=true有效）
};

// Render Abstraction Layer接口基类
class IRALResource 
{
public:
	IRALResource(RALResourceType type) 
		: m_refCount(1)
		, m_resourceType(type)
	{
	}

protected:
	virtual ~IRALResource() = default;

public:
	// 获取资源类型
	RALResourceType GetResourceType() const {
		return m_resourceType;
	}

	// 增加引用计数
	void AddRef()
	{
		m_refCount.fetch_add(1, std::memory_order_acquire);
	}

	// 减少引用计数
	void Release()
	{
		if (m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			delete this;
		}
	}

protected:
	std::atomic<int32_t> m_refCount;
	RALResourceType m_resourceType;
};

// Shader基类
class IRALShader : public IRALResource
{
public:
	IRALShader(RALShaderType shaderType)
		: IRALResource(RALResourceType::Shader)
		, m_shaderType(shaderType)
	{
	}

	virtual ~IRALShader() = default;

	RALShaderType GetShaderType() const
	{
		return m_shaderType;
	}

protected:
	RALShaderType m_shaderType;
};

// 顶点着色器类
class IRALVertexShader : public IRALShader
{
public:
	IRALVertexShader()
		: IRALShader(RALShaderType::Vertex)
	{
	}
	
	virtual ~IRALVertexShader() = default;
};

// 像素着色器类
class IRALPixelShader : public IRALShader
{
public:
	IRALPixelShader()
		: IRALShader(RALShaderType::Pixel)
	{
	}
	
	virtual ~IRALPixelShader() = default;
};

// 网格着色器类
class IRALMeshShader : public IRALShader
{
public:
	IRALMeshShader()
		: IRALShader(RALShaderType::Mesh)
	{
	}
	
	virtual ~IRALMeshShader() = default;
};

// 放大着色器类
class IRALAmplificationShader : public IRALShader
{
public:
	IRALAmplificationShader()
		: IRALShader(RALShaderType::Amplification)
	{
	}
	
	virtual ~IRALAmplificationShader() = default;
};

// 几何着色器类
class IRALGeometryShader : public IRALShader
{
public:
	IRALGeometryShader()
		: IRALShader(RALShaderType::Geometry)
	{
	}
	
	virtual ~IRALGeometryShader() = default;
};

// 计算着色器类
class IRALComputeShader : public IRALShader
{
public:
	IRALComputeShader()
		: IRALShader(RALShaderType::Compute)
	{
	}
	
	virtual ~IRALComputeShader() = default;
};

// 光线生成着色器类
class IRALRayGenShader : public IRALShader
{
public:
	IRALRayGenShader()
		: IRALShader(RALShaderType::RayGen)
	{
	}
	
	virtual ~IRALRayGenShader() = default;
};

// 光线未命中着色器类
class IRALRayMissShader : public IRALShader
{
public:
	IRALRayMissShader()
		: IRALShader(RALShaderType::RayMiss)
	{
	}
	
	virtual ~IRALRayMissShader() = default;
};

// 光线命中组着色器类
class IRALRayHitGroupShader : public IRALShader
{
public:
	IRALRayHitGroupShader()
		: IRALShader(RALShaderType::RayHitGroup)
	{
	}
	
	virtual ~IRALRayHitGroupShader() = default;
};

// 光线可调用着色器类
class IRALRayCallableShader : public IRALShader
{
public:
	IRALRayCallableShader()
		: IRALShader(RALShaderType::RayCallable)
	{
	}
	
	virtual ~IRALRayCallableShader() = default;
};

// 三角形剔除模式（控制哪些朝向的三角形会被剔除）
enum class RALCullMode
{
	None,           // 不剔除任何三角形（所有三角形都将被渲染）
	Front,          // 剔除正面朝向的三角形
	Back,           // 剔除背面朝向的三角形（最常用默认值）
	FrontAndBack    // 剔除所有三角形（仅用于特殊调试场景）
};

// 三角形填充模式（控制光栅化时三角形内部的绘制方式）
enum class RALFillMode
{
	Solid,          // 填充三角形内部（默认渲染模式）
	Wireframe,      // 只绘制三角形的轮廓线（线框模式）
	Point           // 只绘制三角形的顶点（点模式，通常用于调试）
};

// 比较操作（用于深度测试、模板测试等）
enum class RALCompareOp
{
	Never,          // 从不通过比较
	Less,           // 源 < 目标时通过
	Equal,          // 源 == 目标时通过
	LessOrEqual,    // 源 <= 目标时通过
	Greater,        // 源 > 目标时通过
	NotEqual,       // 源 != 目标时通过
	GreaterOrEqual, // 源 >= 目标时通过
	Always          // 始终通过比较
};

// 混合因子（用于计算源和目标颜色的混合权重）
enum class RALBlendFactor
{
	Zero,						// 0.0f
	One,						// 1.0f
	SourceColor,				// 源颜色 (Rs, Gs, Bs, As)
	OneMinusSourceColor,		// 1 - 源颜色 (1-Rs, 1-Gs, 1-Bs, 1-As)
	SourceAlpha,				// 源Alpha (As, As, As, As)
	OneMinusSourceAlpha,		// 1 - 源Alpha (1-As, 1-As, 1-As, 1-As)
	DestinationColor,			// 目标颜色 (Rd, Gd, Bd, Ad)
	OneMinusDestinationColor,	// 1 - 目标颜色 (1-Rd, 1-Gd, 1-Bd, 1-Ad)
	DestinationAlpha,			// 目标Alpha (Ad, Ad, Ad, Ad)
	OneMinusDestinationAlpha,	// 1 - 目标Alpha (1-Ad, 1-Ad, 1-Ad, 1-Ad)
	SourceAlphaSaturate,		// 源Alpha饱和 (min(As, 1-Ad), ...) [仅用于RGB通道]
};

// 混合操作（用于计算源和目标颜色的混合结果）
enum class RALBlendOp {
	Add,            // 源 + 目标
	Subtract,       // 源 - 目标
	ReverseSubtract,// 目标 - 源
	Min,            // min(源, 目标)
	Max             // max(源, 目标)
};

// 颜色通道掩码（控制哪些通道允许写入）
struct RALColorWriteMask {
	bool red = true;
	bool green = true;
	bool blue = true;
	bool alpha = true;
};

// 渲染目标混合状态（每个渲染目标可独立配置）
struct RALBlendState
{
	bool enable = false;                  // 是否启用混合

	// RGB通道混合配置
	RALBlendFactor srcRGB = RALBlendFactor::One;
	RALBlendFactor dstRGB = RALBlendFactor::Zero;
	RALBlendOp opRGB = RALBlendOp::Add;

	// Alpha通道混合配置（可独立于RGB设置）
	RALBlendFactor srcAlpha = RALBlendFactor::One;
	RALBlendFactor dstAlpha = RALBlendFactor::Zero;
	RALBlendOp opAlpha = RALBlendOp::Add;

	RALColorWriteMask writeMask;             // 颜色写入掩码
};

// Pipeline描述
struct RALPipelineDesc
{
	// 光栅化状态
	RALCullMode cullMode;
	RALFillMode fillMode;
	bool depthClipEnable;

	// 深度/模板状态
	bool depthTestEnable;
	bool depthWriteEnable;
	RALCompareOp depthCompareOp;
};

// Viewable资源
class IRALViewableResource : public IRALResource
{
public:
	IRALViewableResource(RALResourceType type) : IRALResource(type)
	{
	}
	
	virtual ~IRALViewableResource() = default;
};

enum ETextureType
{
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube,
	TextureCubeArray,
};

// Texture基类
class IRALTexture : public IRALViewableResource
{
public:
	IRALTexture(ETextureType textureType)
		: IRALViewableResource(RALResourceType::Texture)
		, m_textureType(textureType)
	{

	}
	
	virtual ~IRALTexture() = default;

	// 获取原生资源指针
	virtual void* GetNativeResource() const
	{
		return nullptr;
	}

	// 获取原生Shader resource view
	virtual void* GetNativeShaderResourceView() const
	{
		return nullptr;
	}

protected:
	ETextureType m_textureType;
};

// 顶点缓冲区视图接口
class IVertexBufferView
{
public:
	virtual ~IVertexBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取顶点步长
	virtual uint32_t GetStrideInBytes() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
};

// 索引缓冲区视图接口
class IIndexBufferView {
public:
	virtual ~IIndexBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取索引格式（32位或16位）
	virtual bool Is32Bit() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
};

// 顶点缓冲区接口
class IVertexBuffer : public IRALResource {
public:
	virtual ~IVertexBuffer() = default;
	
	// 创建顶点缓冲区视图
	virtual IVertexBufferView* CreateVertexBufferView(uint32_t stride, uint32_t sizeInBytes) = 0;
};

// 索引缓冲区接口
class IIndexBuffer : public IRALResource {
public:
	virtual ~IIndexBuffer() = default;
	
	// 创建索引缓冲区视图
	virtual IIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytes, bool is32Bit) = 0;
};

// 常量缓冲区视图接口
class IConstBufferView {
public:
	virtual ~IConstBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
};

// 常量缓冲区接口
class IConstBuffer : public IRALResource {
public:
	virtual ~IConstBuffer() = default;
	
	// 创建常量缓冲区视图
	virtual IConstBufferView* CreateConstBufferView(uint32_t sizeInBytes) = 0;

	// 更新常量缓冲区数据
	virtual void Update(const void* data, uint32_t sizeInBytes) = 0;
};

#endif // RALRESOURCE_H