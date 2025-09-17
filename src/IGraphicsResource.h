#pragma once

// 图形资源接口基类
class IGraphicsResource {
public:
    virtual ~IGraphicsResource() = default;
    
    // 获取原生资源指针
    virtual void* GetNativeResource() = 0;
};

// 顶点缓冲区视图接口
class IVertexBufferView {
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
class IVertexBuffer : public IGraphicsResource {
public:
    virtual ~IVertexBuffer() = default;
    
    // 创建顶点缓冲区视图
    virtual IVertexBufferView* CreateVertexBufferView(uint32_t stride, uint32_t sizeInBytes) = 0;
};

// 索引缓冲区接口
class IIndexBuffer : public IGraphicsResource {
public:
    virtual ~IIndexBuffer() = default;
    
    // 创建索引缓冲区视图
    virtual IIndexBufferView* CreateIndexBufferView(uint32_t sizeInBytes, bool is32Bit) = 0;
};