#ifndef RALRESOURCE_H
#define RALRESOURCE_H

#include <cstdint>
#include <atomic>

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


//Render Abstraction Layer接口基类
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
    //获取资源类型
    RALResourceType GetResourceType() const {
        return m_resourceType;
    }

    //增加引用计数
    void AddRef()
    {
        m_refCount.fetch_add(1, std::memory_order_acquire);
    }

    //减少引用计数
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

    RALShaderType GetShaderType() const
    {
        return m_shaderType;
    }

protected:
    RALShaderType m_shaderType;
};

class IRALVertexShader : public IRALShader
{
public:
    IRALVertexShader()
        : IRALShader(RALShaderType::Vertex)
    {
    }
};

class IRALPixelShader : public IRALShader
{
public:
    IRALPixelShader()
        : IRALShader(RALShaderType::Pixel)
    {
    }
};

class IRALMeshShader : public IRALShader
{
public:
    IRALMeshShader()
        : IRALShader(RALShaderType::Mesh)
    {
    }
};


//Viewable资源
class IRALViewableResource : public IRALResource
{
public:
    IRALViewableResource(RALResourceType type) : IRALResource(type)
    {
    }
};

enum ETextureType
{
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
};

//Texture基类
class IRALTexture : public IRALViewableResource
{
public:
    IRALTexture(ETextureType textureType)
        : IRALViewableResource(RALResourceType::Texture)
        , m_textureType(textureType)
    {

    }

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