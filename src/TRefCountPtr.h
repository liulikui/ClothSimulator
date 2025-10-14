#ifndef TREF_COUNT_PTR_H
#define TREF_COUNT_PTR_H

#include <algorithm>  // for std::swap
#include <utility>    // for std::forward

// TRefCountPtr - 支持引用计数的智能指针模板类
// 要求模板参数T必须提供AddRef()和Release()方法

template <typename T>
class TRefCountPtr
{
public:
    // 默认构造函数
    TRefCountPtr() : m_ptr(nullptr) {}
    
    // 从原始指针构造
    explicit TRefCountPtr(T* ptr) : m_ptr(ptr)
    {
        if (m_ptr) 
        {
            m_ptr->AddRef();
        }
    }
    
    // 拷贝构造函数
    TRefCountPtr(const TRefCountPtr<T>& other) : m_ptr(other.m_ptr)
    {
        if (m_ptr) 
        {
            m_ptr->AddRef();
        }
    }
    
    // 移动构造函数
    TRefCountPtr(TRefCountPtr<T>&& other) noexcept : m_ptr(other.m_ptr)
    {
            other.m_ptr = nullptr;
        }
    
        // 析构函数
    ~TRefCountPtr()
    {
        if (m_ptr) 
        {
            m_ptr->Release();
        }
    }
    
    // 拷贝赋值运算符
    TRefCountPtr<T>& operator=(const TRefCountPtr<T>& other)
    {
        if (this != &other) 
        {
            // 先增加新指针的引用计数，避免自赋值导致对象被提前释放
            T* temp = other.m_ptr;
            if (temp) 
            {
                temp->AddRef();
            }
            
            // 释放旧指针
            if (m_ptr) 
            {
                m_ptr->Release();
            }
            
            m_ptr = temp;
        }
        return *this;
    }
    
    // 移动赋值运算符
    TRefCountPtr<T>& operator=(TRefCountPtr<T>&& other) noexcept
    {
        if (this != &other) 
        {
            // 释放旧指针
            if (m_ptr) 
            {
                m_ptr->Release();
            }
            
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }
    
    // 原始指针赋值运算符
    TRefCountPtr<T>& operator=(T* ptr)
    {
        if (m_ptr != ptr) 
        {
            // 先增加新指针的引用计数
            if (ptr)
            {
                ptr->AddRef();
            }
            
            // 释放旧指针
            if (m_ptr)
            {
                m_ptr->Release();
            }
            
            m_ptr = ptr;
        }
        return *this;
    }
    
    // 解引用运算符
    T& operator*() const
    {
        return *m_ptr;
    }
    
    // 箭头运算符
    T* operator->() const
    {
        return m_ptr;
    }
    
    // 获取原始指针
    T* Get() const
    {
        return m_ptr;
    }
    
    // 检查是否为空
    bool IsNull() const
    {
        return m_ptr == nullptr;
    }
    
    // 显式转换为bool
    explicit operator bool() const
    {
        return m_ptr != nullptr;
    }
    
    // 重置指针
    void Reset()
    {
        if (m_ptr)
        {
            m_ptr->Release();
            m_ptr = nullptr;
        }
    }
    
    // 交换两个智能指针
    void Swap(TRefCountPtr<T>& other)
    {
        std::swap(m_ptr, other.m_ptr);
    }
    
private:
    T* m_ptr; // 原始指针
};

// 比较运算符重载
template <typename T>
bool operator==(const TRefCountPtr<T>& lhs, const TRefCountPtr<T>& rhs)
{
    return lhs.Get() == rhs.Get();
}

template <typename T>
bool operator!=(const TRefCountPtr<T>& lhs, const TRefCountPtr<T>& rhs)
{
    return lhs.Get() != rhs.Get();
}

template <typename T>
bool operator<(const TRefCountPtr<T>& lhs, const TRefCountPtr<T>& rhs)
{
    return lhs.Get() < rhs.Get();
}

// 辅助函数：创建TRefCountPtr对象
template <typename T, typename... Args>
TRefCountPtr<T> MakeRefCountPtr(Args&&... args) 
{
    return TRefCountPtr<T>(new T(std::forward<Args>(args)...));
}

#endif // TREF_COUNT_PTR_H