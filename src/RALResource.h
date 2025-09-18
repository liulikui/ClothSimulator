#ifndef RALRESOURCE_H
#define RALRESOURCE_H

#include <cstdint>
#include <atomic>

enum class RALResourceType
<<<<<<< Updated upstream
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
=======
{	NONE,
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

enum class RALShaderType
{	Vertex,
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
>>>>>>> Stashed changes
};

// Render Abstraction Layer接口基类
class IRALResource 
{
public:
<<<<<<< Updated upstream
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
=======
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
>>>>>>> Stashed changes
};

// Shader基类
class IRALShader : public IRALResource
{
public:
<<<<<<< Updated upstream
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
=======
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
>>>>>>> Stashed changes
};

// 顶点着色器类
class IRALVertexShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALVertexShader()
        : IRALShader(RALShaderType::Vertex)
    {
    }
    
    virtual ~IRALVertexShader() = default;
=======
	IRALVertexShader()
		: IRALShader(RALShaderType::Vertex)
	{
	}
	
	virtual ~IRALVertexShader() = default;
>>>>>>> Stashed changes
};

// 像素着色器类
class IRALPixelShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALPixelShader()
        : IRALShader(RALShaderType::Pixel)
    {
    }
    
    virtual ~IRALPixelShader() = default;
=======
	IRALPixelShader()
		: IRALShader(RALShaderType::Pixel)
	{
	}
	
	virtual ~IRALPixelShader() = default;
>>>>>>> Stashed changes
};

// 网格着色器类
class IRALMeshShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALMeshShader()
        : IRALShader(RALShaderType::Mesh)
    {
    }
    
    virtual ~IRALMeshShader() = default;
=======
	IRALMeshShader()
		: IRALShader(RALShaderType::Mesh)
	{
	}
	
	virtual ~IRALMeshShader() = default;
>>>>>>> Stashed changes
};

// 放大着色器类
class IRALAmplificationShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALAmplificationShader()
        : IRALShader(RALShaderType::Amplification)
    {
    }
    
    virtual ~IRALAmplificationShader() = default;
=======
	IRALAmplificationShader()
		: IRALShader(RALShaderType::Amplification)
	{
	}
	
	virtual ~IRALAmplificationShader() = default;
>>>>>>> Stashed changes
};

// 几何着色器类
class IRALGeometryShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALGeometryShader()
        : IRALShader(RALShaderType::Geometry)
    {
    }
    
    virtual ~IRALGeometryShader() = default;
=======
	IRALGeometryShader()
		: IRALShader(RALShaderType::Geometry)
	{
	}
	
	virtual ~IRALGeometryShader() = default;
>>>>>>> Stashed changes
};

// 计算着色器类
class IRALComputeShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALComputeShader()
        : IRALShader(RALShaderType::Compute)
    {
    }
    
    virtual ~IRALComputeShader() = default;
=======
	IRALComputeShader()
		: IRALShader(RALShaderType::Compute)
	{
	}
	
	virtual ~IRALComputeShader() = default;
>>>>>>> Stashed changes
};

// 光线生成着色器类
class IRALRayGenShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALRayGenShader()
        : IRALShader(RALShaderType::RayGen)
    {
    }
    
    virtual ~IRALRayGenShader() = default;
=======
	IRALRayGenShader()
		: IRALShader(RALShaderType::RayGen)
	{
	}
	
	virtual ~IRALRayGenShader() = default;
>>>>>>> Stashed changes
};

// 光线未命中着色器类
class IRALRayMissShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALRayMissShader()
        : IRALShader(RALShaderType::RayMiss)
    {
    }
    
    virtual ~IRALRayMissShader() = default;
=======
	IRALRayMissShader()
		: IRALShader(RALShaderType::RayMiss)
	{
	}
	
	virtual ~IRALRayMissShader() = default;
>>>>>>> Stashed changes
};

// 光线命中组着色器类
class IRALRayHitGroupShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALRayHitGroupShader()
        : IRALShader(RALShaderType::RayHitGroup)
    {
    }
    
    virtual ~IRALRayHitGroupShader() = default;
=======
	IRALRayHitGroupShader()
		: IRALShader(RALShaderType::RayHitGroup)
	{
	}
	
	virtual ~IRALRayHitGroupShader() = default;
>>>>>>> Stashed changes
};

// 光线可调用着色器类
class IRALRayCallableShader : public IRALShader
{
public:
<<<<<<< Updated upstream
    IRALRayCallableShader()
        : IRALShader(RALShaderType::RayCallable)
    {
    }
    
    virtual ~IRALRayCallableShader() = default;
};

=======
	IRALRayCallableShader()
		: IRALShader(RALShaderType::RayCallable)
	{
	}
	
	virtual ~IRALRayCallableShader() = default;
};

// 三角形剔除模式（控制哪些朝向的三角形会被剔除）
enum class RALCullMode {
	None,           // 不剔除任何三角形（所有三角形都将被渲染）
	Front,          // 剔除正面朝向的三角形
	Back,           // 剔除背面朝向的三角形（最常用默认值）
	FrontAndBack    // 剔除所有三角形（仅用于特殊调试场景）
};

// 三角形填充模式（控制光栅化时三角形内部的绘制方式）
enum class FillMode {
	Solid,          // 填充三角形内部（默认渲染模式）
	Wireframe,      // 只绘制三角形的轮廓线（线框模式）
	Point           // 只绘制三角形的顶点（点模式，通常用于调试）
};


// 比较操作（用于深度测试、模板测试等）
enum class RALCompareOp {
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
enum class RALBlendFactor {
	Zero,                  // 0.0f
	One,                   // 1.0f
	SourceColor,           // 源颜色 (Rs, Gs, Bs, As)
	OneMinusSourceColor,   // 1 - 源颜色 (1-Rs, 1-Gs, 1-Bs, 1-As)
	SourceAlpha,           // 源Alpha (As, As, As, As)
	OneMinusSourceAlpha,   // 1 - 源Alpha (1-As, 1-As, 1-As, 1-As)
	DestinationColor,      // 目标颜色 (Rd, Gd, Bd, Ad)
	OneMinusDestinationColor, // 1 - 目标颜色 (1-Rd, 1-Gd, 1-Bd, 1-Ad)
	DestinationAlpha,      // 目标Alpha (Ad, Ad, Ad, Ad)
	OneMinusDestinationAlpha, // 1 - 目标Alpha (1-Ad, 1-Ad, 1-Ad, 1-Ad)
	SourceAlphaSaturate,   // 源Alpha饱和 (min(As, 1-Ad), ...) [仅用于RGB通道]
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
struct RALBlendState {
	bool enable = false;                  // 是否启用混合

	// RGB通道混合配置
	RALBlendFactor srcRGB = BlendFactor::One;
	RALBlendFactor dstRGB = BlendFactor::Zero;
	RALBlendOp opRGB = BlendOp::Add;

	// Alpha通道混合配置（可独立于RGB设置）
	RALBlendFactor srcAlpha = BlendFactor::One;
	RALBlendFactor dstAlpha = BlendFactor::Zero;
	RALBlendOp opAlpha = BlendOp::Add;

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
	CompareOp depthCompareOp;
};
>>>>>>> Stashed changes

// Viewable资源
class IRALViewableResource : public IRALResource
{
public:
<<<<<<< Updated upstream
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
=======
	IRALViewableResource(RALResourceType type) : IRALResource(type)
	{
	}
	
	virtual ~IRALViewableResource() = default;
};

enum ETextureType
{	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube,
	TextureCubeArray,
>>>>>>> Stashed changes
};

// Texture基类
class IRALTexture : public IRALViewableResource
{
public:
<<<<<<< Updated upstream
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
=======
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
>>>>>>> Stashed changes
};

// 顶点缓冲区视图接口
class IVertexBufferView
{
public:
<<<<<<< Updated upstream
    virtual ~IVertexBufferView() = default;
    
    // 获取缓冲区起始地址
    virtual uint64_t GetBufferLocation() const = 0;
    
    // 获取缓冲区大小
    virtual uint32_t GetSizeInBytes() const = 0;
    
    // 获取顶点步长
    virtual uint32_t GetStrideInBytes() const = 0;
    
    // 获取原生视图指针
    virtual void* GetNativeView() = 0;
=======
	virtual ~IVertexBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取顶点步长
	virtual uint32_t GetStrideInBytes() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
>>>>>>> Stashed changes
};

// 索引缓冲区视图接口
class IIndexBufferView {
public:
<<<<<<< Updated upstream
    virtual ~IIndexBufferView() = default;
    
    // 获取缓冲区起始地址
    virtual uint64_t GetBufferLocation() const = 0;
    
    // 获取缓冲区大小
    virtual uint32_t GetSizeInBytes() const = 0;
    
    // 获取索引格式（32位或16位）
    virtual bool Is32Bit() const = 0;
    
    // 获取原生视图指针
    virtual void* GetNativeView() = 0;
=======
	virtual ~IIndexBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取索引格式（32位或16位）
	virtual bool Is32Bit() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
>>>>>>> Stashed changes
};

// 顶点缓冲区接口
class IVertexBuffer : public IRALResource {
public:
<<<<<<< Updated upstream
    virtual ~IVertexBuffer() = default;
    
    // 创建顶点缓冲区视图
    virtual IVertexBufferView* CreateVertexBufferView(uint32_t stride, uint32_t sizeInBytes) = 0;
=======
	virtual ~IVertexBuffer() = default;
	
	// 创建顶点缓冲区视图
	virtual IVertexBufferView* CreateVertexBufferView(uint32_t stride, uint32_t sizeInBytes) = 0;
>>>>>>> Stashed changes
};

// 索引缓冲区接口
class IIndexBuffer : public IRALResource {
public:
<<<<<<< Updated upstream
    virtual ~IIndexBuffer() = default;
    
    // 创建索引缓冲区视图
    virtual IIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytes, bool is32Bit) = 0;
=======
	virtual ~IIndexBuffer() = default;
	
	// 创建索引缓冲区视图
	virtual IIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytes, bool is32Bit) = 0;
>>>>>>> Stashed changes
};

// 常量缓冲区视图接口
class IConstBufferView {
public:
<<<<<<< Updated upstream
    virtual ~IConstBufferView() = default;
    
    // 获取缓冲区起始地址
    virtual uint64_t GetBufferLocation() const = 0;
    
    // 获取缓冲区大小
    virtual uint32_t GetSizeInBytes() const = 0;
    
    // 获取原生视图指针
    virtual void* GetNativeView() = 0;
=======
	virtual ~IConstBufferView() = default;
	
	// 获取缓冲区起始地址
	virtual uint64_t GetBufferLocation() const = 0;
	
	// 获取缓冲区大小
	virtual uint32_t GetSizeInBytes() const = 0;
	
	// 获取原生视图指针
	virtual void* GetNativeView() = 0;
>>>>>>> Stashed changes
};

// 常量缓冲区接口
class IConstBuffer : public IRALResource {
public:
<<<<<<< Updated upstream
    virtual ~IConstBuffer() = default;
    
    // 创建常量缓冲区视图
    virtual IConstBufferView* CreateConstBufferView(uint32_t sizeInBytes) = 0;

    // 更新常量缓冲区数据
    virtual void Update(const void* data, uint32_t sizeInBytes) = 0;
=======
	virtual ~IConstBuffer() = default;
	
	// 创建常量缓冲区视图
	virtual IConstBufferView* CreateConstBufferView(uint32_t sizeInBytes) = 0;

	// 更新常量缓冲区数据
	virtual void Update(const void* data, uint32_t sizeInBytes) = 0;
>>>>>>> Stashed changes
};

#endif // RALRESOURCE_H