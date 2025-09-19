#ifndef RALRESOURCE_H
#define RALRESOURCE_H

#include <cstdint>
#include <atomic>
#include <vector>
#include "DataFormat.h"

// 前向声明
class IRALRootSignature;
class IRALVertexShader;
class IRALPixelShader;
class IRALHullShader;
class IRALDomainShader;
class IRALGeometryShader;
class IRALAmplificationShader;
class IRALMeshShader;
class IRALRayGenerationShader;
class IRALRayMissShader;
class IRALRayHitGroupShader;
class IRALRayCallableShader;
class IRALComputeShader;
class IRALTexture;
class IRALRenderTarget;
class IRALDepthStencilView;
class IRALVertexBuffer;
class IRALIndexBuffer;
class IRALUniformBuffer;
class IRALCommandList;
class IRALViewport;

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

// 单个顶点属性的描述
struct RALVertexAttribute 
{
	RALVertexSemantic semantic;  // 属性语义（如POSITION）
	RALVertexFormat format;      // 数据格式（如Float3）
	uint32_t bufferSlot;      // 绑定的顶点缓冲区索引（支持多缓冲区）
	uint32_t offset;          // 在缓冲区中的字节偏移量
};

// 图元拓扑类型
enum class RALPrimitiveTopologyType
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	LineListAdj,
	LineStripAdj,
	TriangleListAdj,
	TriangleStripAdj
};

// 三角形剔除模式
enum class RALCullMode
{
    None,
    Front,
    Back,
    FrontAndBack
};

// 三角形填充模式
enum class RALFillMode
{
    Solid,
    Wireframe,
    Point
};

// 比较操作
enum class RALCompareOp
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

// 混合因子
enum class RALBlendFactor
{
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationColor,
    OneMinusDestinationColor,
    DestinationAlpha,
    OneMinusDestinationAlpha,
    SourceAlphaSaturate
};

// 混合操作
enum class RALBlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

// 逻辑操作
enum class RALLogicOp
{
    Noop,
    And,
    Or,
    Xor,
    Not,
    Copy,
    CopyInverted,
    AndReverse,
    AndInverted,
    OrReverse,
    OrInverted,
    XorReverse,
    Equiv,
    Nand,
    Nor,
    Set
};

// 模板操作
enum class RALStencilOp
{
    Keep,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap
};

// 模板操作状态
struct RALStencilOpState
{
    RALStencilOp failOp = RALStencilOp::Keep;
    RALStencilOp depthFailOp = RALStencilOp::Keep;
    RALStencilOp passOp = RALStencilOp::Keep;
    RALCompareOp compareFunc = RALCompareOp::Always;
};

// 多重采样描述
struct RALSampleDesc
{
    uint32_t Count = 1;
    uint32_t Quality = 0;
};

// 光栅化状态
struct RasterizerState
{
    RALCullMode cullMode = RALCullMode::Back;
    RALFillMode fillMode = RALFillMode::Solid;
    bool frontCounterClockwise = false;
    float depthBias = 0.0f;
    float depthBiasClamp = 0.0f;
    float slopeScaledDepthBias = 0.0f;
    bool depthClipEnable = true;
    bool multisampleEnable = false;
    bool antialiasedLineEnable = false;
    uint32_t forcedSampleCount = 0;
    bool conservativeRaster = false;
};

// 混合状态
struct RALPipelineBlendState
{
    bool alphaToCoverageEnable = false;
    bool independentBlendEnable = false;
};

// 渲染目标混合状态
struct RALRenderTargetBlendState
{
    bool blendEnable = false;
    bool logicOpEnable = false;
    RALBlendFactor srcBlend = RALBlendFactor::One;
    RALBlendFactor destBlend = RALBlendFactor::Zero;
    RALBlendOp blendOp = RALBlendOp::Add;
    RALBlendFactor srcBlendAlpha = RALBlendFactor::One;
    RALBlendFactor destBlendAlpha = RALBlendFactor::Zero;
    RALBlendOp blendOpAlpha = RALBlendOp::Add;
    RALLogicOp logicOp = RALLogicOp::Noop;
    uint8_t colorWriteMask = 0xF; // RGBA
};

// 深度模板状态
struct DepthStencilState
{
    bool depthEnable = true;
    bool depthWriteMask = true;
    RALCompareOp depthFunc = RALCompareOp::Less;
    bool stencilEnable = false;
    uint8_t stencilReadMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;
    RALStencilOpState frontFace;
    RALStencilOpState backFace;
};

// 图形管线状态描述
struct RALGraphicsPipelineStateDesc
{
    // 输入布局
    std::vector<RALVertexAttribute>* inputLayout = nullptr;
    
    // 根签名
    IRALRootSignature* rootSignature = nullptr;
    
    // 着色器
    IRALVertexShader* vertexShader = nullptr;
    IRALPixelShader* pixelShader = nullptr;
    IRALGeometryShader* geometryShader = nullptr;
    IRALHullShader* hullShader = nullptr;
    IRALDomainShader* domainShader = nullptr;
    
    // 图元拓扑类型
    RALPrimitiveTopologyType primitiveTopologyType = RALPrimitiveTopologyType::TriangleList;
    
    // 光栅化状态
    RasterizerState rasterizerState;
    
    // 混合状态
    RALPipelineBlendState blendState;
    
    // 渲染目标混合状态数组
    std::vector<RALRenderTargetBlendState> renderTargetBlendStates;
    
    // 深度模板状态
    DepthStencilState depthStencilState;
    
    // 渲染目标配置
    uint32_t numRenderTargets = 1;
    DataFormat renderTargetFormats[8] = { DataFormat::R8G8B8A8_UNorm, DataFormat::Undefined, DataFormat::Undefined, DataFormat::Undefined, 
                                        DataFormat::Undefined, DataFormat::Undefined, DataFormat::Undefined, DataFormat::Undefined };
    DataFormat depthStencilFormat = DataFormat::D32_Float;
    
    // 多重采样
    RALSampleDesc sampleDesc;
    
    // 采样掩码
    uint32_t sampleMask = UINT32_MAX;
};

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
	RenderTarget,
	Viewport,
	RootSignature,

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
		: m_refCount(0)
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

	// 获取原生资源指针
	virtual void* GetNativeResource() = 0;

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
class IRALVertexBufferView
{
public:
	virtual ~IRALVertexBufferView() = default;
	
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
class IRALIndexBufferView {
public:
	virtual ~IRALIndexBufferView() = default;
	
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
class IRALVertexBuffer : public IRALResource {
public:
	IRALVertexBuffer()
		: IRALResource(RALResourceType::UniformBuffer)
	{
	}
public:
	virtual ~IRALVertexBuffer() = default;
	
	// 创建顶点缓冲区视图
	virtual IRALVertexBufferView* CreateVertexBufferView(uint32_t stride, uint32_t sizeInBytes) = 0;
};

// 索引缓冲区接口
class IRALIndexBuffer : public IRALResource {
public:
	IRALIndexBuffer()
		: IRALResource(RALResourceType::UniformBuffer)
	{
	}
public:
	virtual ~IRALIndexBuffer() = default;
	
	// 创建索引缓冲区视图
	virtual IRALIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytes, bool is32Bit) = 0;
};

// UniformBuffer视图接口
class IRALUniformBufferView {
public:
	virtual ~IRALUniformBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
};

// 渲染目标视图接口
class IRALRenderTargetView {
public:
	virtual ~IRALRenderTargetView() = default;
	
	// 获取原生渲染目标视图指针
	virtual void* GetNativeRenderTargetView() const = 0;
};

// 深度模板视图接口
class IRALDepthStencilView {
public:
	virtual ~IRALDepthStencilView() = default;
	
	// 获取深度模板视图宽度
	virtual uint32_t GetWidth() const = 0;
	
	// 获取深度模板视图高度
	virtual uint32_t GetHeight() const = 0;
	
	// 获取深度模板视图格式
	virtual DataFormat GetFormat() const = 0;
	
	// 清除深度模板视图
	virtual void Clear(float depth, uint8_t stencil) = 0;
	
	// 获取原生深度模板视图指针
	virtual void* GetNativeDepthStencilView() const = 0;
};

// UniformBuffer接口
class IRALUniformBuffer : public IRALResource {
public:
	IRALUniformBuffer()
		: IRALResource(RALResourceType::UniformBuffer)
	{
	}
public:
	virtual ~IRALUniformBuffer() = default;
	
	// 创建UniformBuffer视图
	virtual IRALUniformBufferView* CreateUniformBufferView(uint32_t sizeInBytes) = 0;

	// 更新UniformBuffer数据
	virtual void Update(const void* data, uint32_t sizeInBytes) = 0;
};

// RootSignature参数类型枚举
enum class RALRootParameterType
{
	Constant,
	ConstantBufferView,
	ShaderResourceView,
	UnorderedAccessView,
	DescriptorTable,
	Invalid
};

// 根描述符（用于根CBV/SRV/UAV）
struct RALRootDescriptor
{
	uint32_t ShaderRegister;
	uint32_t RegisterSpace;
};

// 根描述符表范围
struct RALRootDescriptorTableRange
{
	uint32_t NumDescriptors;
	uint32_t BaseShaderRegister;
	uint32_t RegisterSpace;
};

// 根描述符表
struct RALRootDescriptorTable
{
	std::vector<RALRootDescriptorTableRange> Ranges;
};

// 着色器可见性枚举
enum class RALShaderVisibility
{
	All,
	Vertex,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Amplification,
	Mesh
};

// 根签名参数结构体
struct RALRootParameter
{
	RALRootParameterType Type;
	
	// 使用结构体替代union，避免隐式删除析构函数的警告
	struct
	{
		uint32_t Constants[3]; // ShaderRegister, RegisterSpace, Num32BitValues
		RALRootDescriptor Descriptor;
		RALRootDescriptorTable DescriptorTable;
	} Data;
	
	RALShaderVisibility ShaderVisibility;
};

// 初始化常量根参数
static inline void InitAsConstants(RALRootParameter& param, uint32_t shaderRegister, uint32_t registerSpace, uint32_t num32BitValues, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::Constant;
	param.Data.Constants[0] = shaderRegister;
	param.Data.Constants[1] = registerSpace;
	param.Data.Constants[2] = num32BitValues;
	param.ShaderVisibility = visibility;
}

// 初始化CBV根参数
static inline void InitAsConstantBufferView(RALRootParameter& param, uint32_t shaderRegister, uint32_t registerSpace, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::ConstantBufferView;
	param.Data.Descriptor.ShaderRegister = shaderRegister;
	param.Data.Descriptor.RegisterSpace = registerSpace;
	param.ShaderVisibility = visibility;
}

// 初始化SRV根参数
static inline void InitAsShaderResourceView(RALRootParameter& param, uint32_t shaderRegister, uint32_t registerSpace, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::ShaderResourceView;
	param.Data.Descriptor.ShaderRegister = shaderRegister;
	param.Data.Descriptor.RegisterSpace = registerSpace;
	param.ShaderVisibility = visibility;
}

// 初始化UAV根参数
static inline void InitAsUnorderedAccessView(RALRootParameter& param, uint32_t shaderRegister, uint32_t registerSpace, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::UnorderedAccessView;
	param.Data.Descriptor.ShaderRegister = shaderRegister;
	param.Data.Descriptor.RegisterSpace = registerSpace;
	param.ShaderVisibility = visibility;
}

// 初始化描述符表根参数
static inline void InitAsDescriptorTable(RALRootParameter& param, const std::vector<RALRootDescriptorTableRange>& ranges, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::DescriptorTable;
	param.Data.DescriptorTable.Ranges = ranges;
	param.ShaderVisibility = visibility;
}

// RootSignature接口
class IRALRootSignature : public IRALResource
{
public:
	IRALRootSignature()
		: IRALResource(RALResourceType::RootSignature)
	{
	}

	virtual ~IRALRootSignature() = default;
};

// Graphics Pipeline State接口
class IRALGraphicsPipelineState : public IRALResource
{
public:
	IRALGraphicsPipelineState()
		: IRALResource(RALResourceType::GraphicsPipelineState)
	{
	}

	virtual ~IRALGraphicsPipelineState() = default;
};

// RenderTarget基类
class IRALRenderTarget : public IRALViewableResource
{
public:
	IRALRenderTarget()
		: IRALViewableResource(RALResourceType::RenderTarget)
	{
	}
	
	virtual ~IRALRenderTarget() = default;

	// 获取渲染目标宽度
	virtual uint32_t GetWidth() const = 0;
	
	// 获取渲染目标高度
	virtual uint32_t GetHeight() const = 0;
	
	// 获取渲染目标格式
	virtual DataFormat GetFormat() const = 0;
	
	// 清除渲染目标
	virtual void Clear(const float color[4]) = 0;
	
	// 获取原生渲染目标视图
	virtual void* GetNativeRenderTargetView() const = 0;
	
	// 创建渲染目标视图
	virtual IRALRenderTargetView* CreateRenderTargetView() = 0;
	
	// 创建深度模板视图
	virtual IRALDepthStencilView* CreateDepthStencilView() = 0;
};

#endif // RALRESOURCE_H