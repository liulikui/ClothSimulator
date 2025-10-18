#ifndef RALRESOURCE_H
#define RALRESOURCE_H

#include <cstdint>
#include <atomic>
#include <vector>
#include "RALDataFormat.h"

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
class IRALDepthStencil;
class IRALVertexBuffer;
class IRALIndexBuffer;
class IRALConstBuffer;
class IRALCommandList;
class IRALViewport;
class IRALDepthStencilView;
class IRALRenderTargetView;
class IRALShaderResourceView;

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
    
	// 构造函数
	RALVertexAttribute() : semantic(RALVertexSemantic::Position), format(RALVertexFormat::Float3), bufferSlot(0), offset(0) {}
	RALVertexAttribute(RALVertexSemantic s, RALVertexFormat f, uint32_t slot, uint32_t off) 
		: semantic(s), format(f), bufferSlot(slot), offset(off) {}
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
    RALStencilOp failOp;
    RALStencilOp depthFailOp;
    RALStencilOp passOp;
    RALCompareOp compareFunc;
    
    // 构造函数
    RALStencilOpState() 
        : failOp(RALStencilOp::Keep), depthFailOp(RALStencilOp::Keep), 
          passOp(RALStencilOp::Keep), compareFunc(RALCompareOp::Always) {}
    RALStencilOpState(RALStencilOp fail, RALStencilOp depthFail, RALStencilOp pass, RALCompareOp func) 
        : failOp(fail), depthFailOp(depthFail), passOp(pass), compareFunc(func) {}
};

// 多重采样描述
struct RALSampleDesc
{
    uint32_t Count;
    uint32_t Quality;
    
    // 构造函数
    RALSampleDesc() : Count(1), Quality(0) {}
    RALSampleDesc(uint32_t count, uint32_t quality) : Count(count), Quality(quality) {}
};

// 光栅化状态
struct RasterizerState
{
    RALCullMode cullMode;
    RALFillMode fillMode;
    bool frontCounterClockwise;
    float depthBias;
    float depthBiasClamp;
    float slopeScaledDepthBias;
    bool depthClipEnable;
    bool multisampleEnable;
    bool antialiasedLineEnable;
    uint32_t forcedSampleCount;
    bool conservativeRaster;
    
    // 构造函数
    RasterizerState() 
        : cullMode(RALCullMode::Back), fillMode(RALFillMode::Solid), frontCounterClockwise(false),
          depthBias(0.0f), depthBiasClamp(0.0f), slopeScaledDepthBias(0.0f),
          depthClipEnable(true), multisampleEnable(false), antialiasedLineEnable(false),
          forcedSampleCount(0), conservativeRaster(false) {}
};

// 混合状态
struct RALPipelineBlendState
{
    bool alphaToCoverageEnable;
    bool independentBlendEnable;
    
    // 构造函数
    RALPipelineBlendState() : alphaToCoverageEnable(false), independentBlendEnable(false) {}
    RALPipelineBlendState(bool alphaCoverage, bool independentBlend) 
        : alphaToCoverageEnable(alphaCoverage), independentBlendEnable(independentBlend) {}
};

// 渲染目标混合状态
struct RALRenderTargetBlendState
{
    bool blendEnable;
    bool logicOpEnable;
    RALBlendFactor srcBlend;
    RALBlendFactor destBlend;
    RALBlendOp blendOp;
    RALBlendFactor srcBlendAlpha;
    RALBlendFactor destBlendAlpha;
    RALBlendOp blendOpAlpha;
    RALLogicOp logicOp;
    uint8_t colorWriteMask;
    
    // 构造函数
    RALRenderTargetBlendState() 
        : blendEnable(false), logicOpEnable(false), 
          srcBlend(RALBlendFactor::One), destBlend(RALBlendFactor::Zero), blendOp(RALBlendOp::Add),
          srcBlendAlpha(RALBlendFactor::One), destBlendAlpha(RALBlendFactor::Zero), blendOpAlpha(RALBlendOp::Add),
          logicOp(RALLogicOp::Noop), colorWriteMask(0xF) {}
};

// 清除值结构体 - 封装D3D12_CLEAR_VALUE
struct RALClearValue
{
    RALDataFormat format;        // 纹理格式
    union
    {
        float color[4];           // 渲染目标清除颜色 (RGBA)
        struct
        {
            float depth;          // 深度缓冲区清除值
            uint8_t stencil;      // 模板缓冲区清除值
        } depthStencil;           // 深度/模板清除值
    } clearValue;
    
    // 构造函数 - 默认构造
    RALClearValue() : format(RALDataFormat::Undefined) 
    {
        clearValue.color[0] = 0.0f;
        clearValue.color[1] = 0.0f;
        clearValue.color[2] = 0.0f;
        clearValue.color[3] = 1.0f;
    }
    
    // 构造函数 - 渲染目标清除值
    RALClearValue(RALDataFormat fmt, float r, float g, float b, float a) : format(fmt) 
    {
        clearValue.color[0] = r;
        clearValue.color[1] = g;
        clearValue.color[2] = b;
        clearValue.color[3] = a;
    }
    
    // 构造函数 - 深度模板清除值
    RALClearValue(RALDataFormat fmt, float d, uint8_t s) : format(fmt) 
    {
        clearValue.depthStencil.depth = d;
        clearValue.depthStencil.stencil = s;
    }
};

// 深度模板状态
struct DepthStencilState
{
    bool depthEnable;
    bool depthWriteMask;
    RALCompareOp depthFunc;
    bool stencilEnable;
    uint8_t stencilReadMask;
    uint8_t stencilWriteMask;
    RALStencilOpState frontFace;
    RALStencilOpState backFace;
    
    // 构造函数
    DepthStencilState() 
        : depthEnable(true), depthWriteMask(true), depthFunc(RALCompareOp::Less),
          stencilEnable(false), stencilReadMask(0xFF), stencilWriteMask(0xFF),
          frontFace(), backFace() {}
};

// 图形管线状态描述
struct RALGraphicsPipelineStateDesc
{
    // 输入布局
    std::vector<RALVertexAttribute>* inputLayout;
    
    // 根签名
    IRALRootSignature* rootSignature;
    
    // 着色器
    IRALVertexShader* vertexShader;
    IRALPixelShader* pixelShader;
    IRALGeometryShader* geometryShader;
    IRALHullShader* hullShader;
    IRALDomainShader* domainShader;
    
    // 图元拓扑类型
    RALPrimitiveTopologyType primitiveTopologyType;
    
    // 光栅化状态
    RasterizerState rasterizerState;
    
    // 混合状态
    RALPipelineBlendState blendState;
    
    // 渲染目标混合状态数组
    std::vector<RALRenderTargetBlendState> renderTargetBlendStates;
    
    // 深度模板状态
    DepthStencilState depthStencilState;
    
    // 渲染目标配置
    uint32_t numRenderTargets;
    RALDataFormat renderTargetFormats[8];
    RALDataFormat depthStencilFormat;
    
    // 多重采样
    RALSampleDesc sampleDesc;
    
    // 采样掩码
    uint32_t sampleMask;
    
    // 构造函数
    RALGraphicsPipelineStateDesc() 
        : inputLayout(nullptr), rootSignature(nullptr),
          vertexShader(nullptr), pixelShader(nullptr), geometryShader(nullptr),
          hullShader(nullptr), domainShader(nullptr),
          primitiveTopologyType(RALPrimitiveTopologyType::TriangleList),
          rasterizerState(), blendState(), renderTargetBlendStates(),
          depthStencilState(), numRenderTargets(1),
          depthStencilFormat(RALDataFormat::D32_Float),
          sampleDesc(), sampleMask(UINT32_MAX)
    {
        // 初始化渲染目标格式数组
        renderTargetFormats[0] = RALDataFormat::R8G8B8A8_UNorm;
        for (int i = 1; i < 8; i++)
        {
            renderTargetFormats[i] = RALDataFormat::Undefined;
        }
    }
};

// 资源屏障类型
enum class RALResourceBarrierType
{
	Transition,
	Aliasing,
	UnorderedAccess
};

// 资源状态
enum class RALResourceState
{
	// 初始/通用状态
	Common,

	// 复制相关
	CopySource,
	CopyDest,

	// 顶点/索引缓冲区
	VertexBuffer,
	IndexBuffer,

	// 着色器资源
	ShaderResource,

	// 渲染目标/深度缓冲
	RenderTarget,
	DepthStencil,
};

// Render Abstraction Layer资源类型
enum class RALResourceType
{
	NONE,
	ConstBuffer,
	ConstBufferLayout,
	VertexBuffer,
	IndexBuffer,
	Shader,
	GraphicsPipelineState,
	ComputePipelineState,
	RayTracingPipelineState,
	Texture,
	RenderTarget,
	DepthStencil,
	Viewport,
	RootSignature,
	DepthStencilView,
	RenderTargetView,
	ShaderResourceView,

	Count
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
    
	// 构造函数
	RALVertexBufferBinding() : stride(0), isInstanceData(false), instanceStepRate(0) {}
	RALVertexBufferBinding(uint32_t s, bool instanceData, uint32_t stepRate) 
		: stride(s), isInstanceData(instanceData), instanceStepRate(stepRate) {}
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
	RALResourceType GetResourceType() const 
	{
		return m_resourceType;
	}

	// 获取资源状态
	RALResourceState GetResourceState() const
	{
		return m_resourceState;
	}

	// 设置资源状态
	void SetResourceState(RALResourceState state)
	{
		m_resourceState = state;
	}

	// 获取原生资源指针
	virtual void* GetNativeResource() const = 0;

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
	RALResourceState m_resourceState;
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
	bool red;
	bool green;
	bool blue;
	bool alpha;
    
	// 构造函数
	RALColorWriteMask() : red(true), green(true), blue(true), alpha(true) {}
	RALColorWriteMask(bool r, bool g, bool b, bool a) : red(r), green(g), blue(b), alpha(a) {}
};

// 渲染目标混合状态（每个渲染目标可独立配置）
struct RALBlendState
{
	bool enable;                  // 是否启用混合

	// RGB通道混合配置
	RALBlendFactor srcRGB;
	RALBlendFactor dstRGB;
	RALBlendOp opRGB;

	// Alpha通道混合配置（可独立于RGB设置）
	RALBlendFactor srcAlpha;
	RALBlendFactor dstAlpha;
	RALBlendOp opAlpha;

	RALColorWriteMask writeMask;             // 颜色写入掩码
    
	// 构造函数
	RALBlendState() 
		: enable(false), 
		  srcRGB(RALBlendFactor::One), dstRGB(RALBlendFactor::Zero), opRGB(RALBlendOp::Add),
		  srcAlpha(RALBlendFactor::One), dstAlpha(RALBlendFactor::Zero), opAlpha(RALBlendOp::Add),
		  writeMask() {}
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
    
	// 构造函数
	RALPipelineDesc() 
		: cullMode(RALCullMode::Back), fillMode(RALFillMode::Solid), depthClipEnable(true),
		  depthTestEnable(true), depthWriteEnable(true), depthCompareOp(RALCompareOp::Less) {}
	RALPipelineDesc(RALCullMode cm, RALFillMode fm, bool clip, bool test, bool write, RALCompareOp op) 
		: cullMode(cm), fillMode(fm), depthClipEnable(clip),
		  depthTestEnable(test), depthWriteEnable(write), depthCompareOp(op) {}
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
class IRALTexture : public IRALResource
{
public:
	IRALTexture(ETextureType textureType)
		: IRALResource(RALResourceType::Texture)
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

struct RALRange
{
	uint64_t begin;
	uint64_t end;
    
	// 构造函数
	RALRange() : begin(0), end(0) {}
	RALRange(uint64_t b, uint64_t e) : begin(b), end(e) {}
};

class IRALBuffer : public IRALResource
{
public:
	IRALBuffer(RALResourceType type, uint32_t size)
		: IRALResource(type)
		, m_size(size)
	{
	}

	uint32_t GetSize() const
	{
		return m_size;
	}

protected:
	uint32_t m_size;
};

// 顶点缓冲区接口
class IRALVertexBuffer : public IRALBuffer
{
public:
	IRALVertexBuffer(uint32_t size)
		: IRALBuffer(RALResourceType::VertexBuffer, size)
	{
	}

	virtual ~IRALVertexBuffer() = default;
};

// 索引缓冲区接口
class IRALIndexBuffer : public IRALBuffer
{
public:
	IRALIndexBuffer(uint32_t count, uint32_t size, bool is32BitIndex)
		: IRALBuffer(RALResourceType::IndexBuffer, size)
		, m_count(count)
		, m_is32BitIndex(is32BitIndex)
	{
	}

	virtual ~IRALIndexBuffer() = default;

	uint32_t GetIndexCount() const
	{
		return m_count;
	}
	bool Is32BitIndex() const
	{
		return m_is32BitIndex;
	}

protected:
	uint32_t m_count;
	bool m_is32BitIndex;
};

// ConstBuffer接口
class IRALConstBuffer : public IRALBuffer
{
public:
	IRALConstBuffer(uint32_t size)
		: IRALBuffer(RALResourceType::ConstBuffer, size)
	{
	}

	virtual ~IRALConstBuffer() = default;

	virtual bool Map(void** ppData) = 0;

	virtual void Unmap() = 0;
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
    
	// 构造函数
	RALRootDescriptor() : ShaderRegister(0), RegisterSpace(0) {}
	RALRootDescriptor(uint32_t reg, uint32_t space) : ShaderRegister(reg), RegisterSpace(space) {}
};

// 描述符范围类型枚举
enum class RALDescriptorRangeType
{
    SRV,
    UAV,
    CBV,
    Sampler
};

// 根描述符表范围
struct RALRootDescriptorTableRange
{
    RALDescriptorRangeType Type; // 新增：描述符范围类型
    uint32_t NumDescriptors;
    uint32_t BaseShaderRegister;
    uint32_t RegisterSpace;
    
	// 构造函数
	RALRootDescriptorTableRange() 
		: Type(RALDescriptorRangeType::SRV), NumDescriptors(0), BaseShaderRegister(0), RegisterSpace(0) {}
	RALRootDescriptorTableRange(RALDescriptorRangeType type, uint32_t num, uint32_t baseReg, uint32_t space) 
		: Type(type), NumDescriptors(num), BaseShaderRegister(baseReg), RegisterSpace(space) {}
};

// 根描述符表
struct RALRootDescriptorTable
{
	std::vector<RALRootDescriptorTableRange> Ranges;
    
	// 构造函数
	RALRootDescriptorTable() : Ranges() {}
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
    
	// 构造函数
	RALRootParameter() 
		: Type(RALRootParameterType::Invalid), 
		  ShaderVisibility(RALShaderVisibility::All)
	{
		// 初始化常量数组
		Data.Constants[0] = 0;
		Data.Constants[1] = 0;
		Data.Constants[2] = 0;
		// Data.Descriptor和Data.DescriptorTable会使用它们自己的默认构造函数
	}
};

// 初始化常量根参数
static inline void InitAsConstants(RALRootParameter& param, 
	uint32_t shaderRegister, uint32_t registerSpace, uint32_t num32BitValues, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::Constant;
	param.Data.Constants[0] = shaderRegister;
	param.Data.Constants[1] = registerSpace;
	param.Data.Constants[2] = num32BitValues;
	param.ShaderVisibility = visibility;
}

// 初始化CBV根参数
static inline void InitAsConstantBufferView(RALRootParameter& param, 
	uint32_t shaderRegister, uint32_t registerSpace, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::ConstantBufferView;
	param.Data.Descriptor.ShaderRegister = shaderRegister;
	param.Data.Descriptor.RegisterSpace = registerSpace;
	param.ShaderVisibility = visibility;
}

// 初始化SRV根参数
static inline void InitAsShaderResourceView(RALRootParameter& param, 
	uint32_t shaderRegister, uint32_t registerSpace, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::ShaderResourceView;
	param.Data.Descriptor.ShaderRegister = shaderRegister;
	param.Data.Descriptor.RegisterSpace = registerSpace;
	param.ShaderVisibility = visibility;
}

// 初始化UAV根参数
static inline void InitAsUnorderedAccessView(RALRootParameter& param, 
	uint32_t shaderRegister, uint32_t registerSpace, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::UnorderedAccessView;
	param.Data.Descriptor.ShaderRegister = shaderRegister;
	param.Data.Descriptor.RegisterSpace = registerSpace;
	param.ShaderVisibility = visibility;
}

// 初始化描述符表根参数
static inline void InitAsDescriptorTable(RALRootParameter& param, 
	const std::vector<RALRootDescriptorTableRange>& ranges, RALShaderVisibility visibility)
{
	param.Type = RALRootParameterType::DescriptorTable;
	param.Data.DescriptorTable.Ranges = ranges;
	param.ShaderVisibility = visibility;
}

// 采样器过滤模式枚举
enum class RALFilter
{
    MinMagMipPoint,
    MinMagPointMipLinear,
    MinPointMagLinearMipPoint,
    MinPointMagMipLinear,
    MinLinearMagMipPoint,
    MinLinearMagPointMipLinear,
    MinMagLinearMipPoint,
    MinMagMipLinear,
    Anisotropic,
    ComparisonMinMagMipPoint,
    ComparisonMinMagPointMipLinear,
    ComparisonMinPointMagLinearMipPoint,
    ComparisonMinPointMagMipLinear,
    ComparisonMinLinearMagMipPoint,
    ComparisonMinLinearMagPointMipLinear,
    ComparisonMinMagLinearMipPoint,
    ComparisonMinMagMipLinear,
    ComparisonAnisotropic
};

// 纹理寻址模式枚举
enum class RALTextureAddressMode
{
    Wrap,
    Mirror,
    Clamp,
    Border,
    MirrorOnce
};

// 比较函数枚举
enum class RALComparisonFunc
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

// 静态边界颜色枚举
enum class RALStaticBorderColor
{
    TransparentBlack,
    OpaqueBlack,
    OpaqueWhite
};

// 根签名标志枚举
enum class RALRootSignatureFlags : uint32_t
{
    None = 0,
    AllowInputAssemblerInputLayout = 1 << 0,
    AllowStreamOutput = 1 << 1,
    DenyVertexShaderResource = 1 << 2,
    DenyHullShaderResource = 1 << 3,
    DenyDomainShaderResource = 1 << 4,
    DenyGeometryShaderResource = 1 << 5,
    DenyPixelShaderResource = 1 << 6,
    AllowFeedbackTextureLOD = 1 << 7
};

// 静态采样器结构体
struct RALStaticSampler
{
    RALFilter Filter;
    RALTextureAddressMode AddressU;
    RALTextureAddressMode AddressV;
    RALTextureAddressMode AddressW;
    float MipLODBias;
    uint32_t MaxAnisotropy;
    RALComparisonFunc ComparisonFunc;
    RALStaticBorderColor BorderColor;
    float MinLOD;
    float MaxLOD;
    uint32_t ShaderRegister;
    uint32_t RegisterSpace;
    RALShaderVisibility ShaderVisibility;
    
	// 构造函数
	RALStaticSampler() 
		: Filter(RALFilter::MinMagMipLinear), 
		  AddressU(RALTextureAddressMode::Wrap), 
		  AddressV(RALTextureAddressMode::Wrap), 
		  AddressW(RALTextureAddressMode::Wrap),
		  MipLODBias(0.0f), MaxAnisotropy(1), ComparisonFunc(RALComparisonFunc::Always),
		  BorderColor(RALStaticBorderColor::TransparentBlack), MinLOD(0.0f), MaxLOD(3.402823466e+38f),
		  ShaderRegister(0), RegisterSpace(0), ShaderVisibility(RALShaderVisibility::All) {}
};

// 初始化静态采样器
static inline void InitStaticSampler(RALStaticSampler& sampler, 
	RALFilter filter, 
	RALTextureAddressMode addressU, 
	RALTextureAddressMode addressV, 
	RALTextureAddressMode addressW, 
	float mipLODBias, 
	uint32_t maxAnisotropy, 
	RALComparisonFunc comparisonFunc, 
	RALStaticBorderColor borderColor, 
	float minLOD, 
	float maxLOD, 
	uint32_t shaderRegister, 
	uint32_t registerSpace, 
	RALShaderVisibility visibility)
{
    sampler.Filter = filter;
    sampler.AddressU = addressU;
    sampler.AddressV = addressV;
    sampler.AddressW = addressW;
    sampler.MipLODBias = mipLODBias;
    sampler.MaxAnisotropy = maxAnisotropy;
    sampler.ComparisonFunc = comparisonFunc;
    sampler.BorderColor = borderColor;
    sampler.MinLOD = minLOD;
    sampler.MaxLOD = maxLOD;
    sampler.ShaderRegister = shaderRegister;
    sampler.RegisterSpace = registerSpace;
    sampler.ShaderVisibility = visibility;
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
class IRALRenderTarget : public IRALResource
{
public:
	IRALRenderTarget(uint32_t width, uint32_t height, RALDataFormat format)
		: IRALResource(RALResourceType::RenderTarget)
		, m_width(width)
		, m_height(height)
		, m_format(format)
	{
	}
	
	virtual ~IRALRenderTarget() = default;

	// 获取宽度
	uint32_t GetWidth() const
	{
		return m_width;
	}
	
	// 获取高度
	uint32_t GetHeight() const
	{
		return m_height;
	}
	
	// 获取格式
	RALDataFormat GetFormat() const
	{
		return m_format;
	}

protected:
	uint32_t m_width;
	uint32_t m_height;
	RALDataFormat m_format;
};

// DepthStencil基类
class IRALDepthStencil : public IRALResource
{
public:
	IRALDepthStencil(uint32_t width, uint32_t height, RALDataFormat format)
		: IRALResource(RALResourceType::DepthStencil)
		, m_width(width)
		, m_height(height)
		, m_format(format)
	{
	}

	virtual ~IRALDepthStencil() = default;

	// 获取宽度
	uint32_t GetWidth() const
	{
		return m_width;
	}

	// 获取高度
	uint32_t GetHeight() const
	{
		return m_height;
	}

	// 获取格式
	RALDataFormat GetFormat() const
	{
		return m_format;
	}

protected:
	uint32_t m_width;
	uint32_t m_height;
	RALDataFormat m_format;
};

// 深度模板视图接口
class IRALDepthStencilView : public IRALResource
{
public:
	IRALDepthStencilView()
		: IRALResource(RALResourceType::DepthStencilView)
	{
	}

	virtual ~IRALDepthStencilView() = default;

	// 获取关联的深度模板资源
	virtual IRALDepthStencil* GetDepthStencil() const = 0;

	// 获取原生深度模板视图
	virtual void* GetNativeDepthStencilView() const = 0;
};

// 深度模板视图描述符（跨平台封装）
struct RALDepthStencilViewDesc
{
    RALDataFormat format;
    uint32_t mipSlice;
    
	// 构造函数
	RALDepthStencilViewDesc() : format(RALDataFormat::Undefined), mipSlice(0) {}
	RALDepthStencilViewDesc(RALDataFormat fmt, uint32_t mip) : format(fmt), mipSlice(mip) {}
};

// 渲染目标视图描述符（跨平台封装）
struct RALRenderTargetViewDesc
{
    RALDataFormat format;
    uint32_t mipSlice;
    uint32_t planeSlice;
    
	// 构造函数
	RALRenderTargetViewDesc() : format(RALDataFormat::Undefined), mipSlice(0), planeSlice(0) {}
	RALRenderTargetViewDesc(RALDataFormat fmt, uint32_t mip, uint32_t plane) 
		: format(fmt), mipSlice(mip), planeSlice(plane) {}
};

// 渲染目标视图接口
class IRALRenderTargetView : public IRALResource
{
public:
	IRALRenderTargetView()
		: IRALResource(RALResourceType::RenderTargetView)
	{
	}

	virtual ~IRALRenderTargetView() = default;

	// 获取关联的渲染目标资源
	virtual IRALRenderTarget* GetRenderTarget() const = 0;

	// 获取原生渲染目标视图
	virtual void* GetNativeRenderTargetView() const = 0;
};

// 着色器资源视图描述符（跨平台封装）
struct RALShaderResourceViewDesc
{
    RALDataFormat format;
    uint32_t mostDetailedMip;
    uint32_t mipLevels;
    uint32_t firstArraySlice;
    uint32_t arraySize;
    
	// 构造函数
	RALShaderResourceViewDesc() 
		: format(RALDataFormat::Undefined), mostDetailedMip(0), 
		  mipLevels(1), firstArraySlice(0), arraySize(1) {}
	RALShaderResourceViewDesc(RALDataFormat fmt, uint32_t detailedMip, uint32_t levels, uint32_t firstSlice, uint32_t size) 
		: format(fmt), mostDetailedMip(detailedMip), mipLevels(levels), 
		  firstArraySlice(firstSlice), arraySize(size) {}
};

// 着色器资源视图接口
class IRALShaderResourceView : public IRALResource
{
public:
	IRALShaderResourceView()
		: IRALResource(RALResourceType::ShaderResourceView)
	{
	}

	virtual ~IRALShaderResourceView() = default;

	// 获取关联的资源（可能是纹理、缓冲区等）
	virtual IRALResource* GetResource() const = 0;

	// 获取原生着色器资源视图
	virtual void* GetNativeShaderResourceView() const = 0;
};

#endif // RALRESOURCE_H