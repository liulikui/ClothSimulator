#ifndef RAL_DATA_FORMAT_H
#define RAL_DATA_FORMAT_H

#include <cstdint>

// 跨平台数据格式枚举（覆盖顶点属性、纹理、缓冲区等通用格式）
enum class RALDataFormat : uint32_t 
{
    // 未定义格式
    Undefined,

    // 单通道8位格式（按DXGI格式组织顺序）
    R8_Typeless,           // 单通道8位无类型
    R8_UInt,               // 单通道8位无符号整数
    R8_SInt,               // 单通道8位有符号整数
    R8_UNorm,              // 单通道8位无符号归一化（[0,1]）
    R8_SNorm,              // 单通道8位有符号归一化（[-1,1]）

    // 单通道16位格式
    R16_Typeless,          // 单通道16位无类型
    R16_UInt,              // 单通道16位无符号整数
    R16_SInt,              // 单通道16位有符号整数
    R16_UNorm,             // 单通道16位无符号归一化
    R16_SNorm,             // 单通道16位有符号归一化
    R16_Float,             // 单通道16位浮点数（半精度）

    // 单通道32位格式
    R32_Typeless,          // 单通道32位无类型（对应DXGI_FORMAT_R32_TYPELESS）
    R32_UInt,              // 单通道32位无符号整数
    R32_SInt,              // 单通道32位有符号整数
    R32_Float,             // 单通道32位浮点数（单精度）

    // 双通道8位格式
    R8G8_Typeless,         // 双通道8位无类型
    R8G8_UInt,             // 双8位无符号整数
    R8G8_SInt,             // 双8位有符号整数
    R8G8_UNorm,            // 双8位无符号归一化
    R8G8_SNorm,            // 双8位有符号归一化

    // 双通道16位格式
    R16G16_Typeless,       // 双通道16位无类型
    R16G16_UInt,           // 双16位无符号整数
    R16G16_SInt,           // 双16位有符号整数
    R16G16_UNorm,          // 双16位无符号归一化
    R16G16_SNorm,          // 双16位有符号归一化
    R16G16_Float,          // 双16位浮点数

    // 双通道32位格式
    R32G32_Typeless,       // 双通道32位无类型
    R32G32_UInt,           // 双32位无符号整数
    R32G32_SInt,           // 双32位有符号整数
    R32G32_Float,          // 双32位浮点数

    // 三通道格式（注意：某些API对3通道格式支持有限）
    R32G32B32_Typeless,    // 三通道32位无类型
    R32G32B32_Float,       // 三32位浮点数
    // 注意：DXGI不直接支持8位三通道格式的typeless版本
    R8G8B8_UInt,           // 三8位无符号整数
    R8G8B8_SInt,           // 三8位有符号整数
    R8G8B8_UNorm,          // 三8位无符号归一化
    R8G8B8_SNorm,          // 三8位有符号归一化

    // 四通道8位格式 - RGBA顺序
    R8G8B8A8_Typeless,     // 四通道8位无类型
    R8G8B8A8_UInt,         // 四8位无符号整数
    R8G8B8A8_SInt,         // 四8位有符号整数
    R8G8B8A8_UNorm,        // 四8位无符号归一化（常用RGBA颜色）
    R8G8B8A8_SNorm,        // 四8位有符号归一化
    R8G8B8A8_SRGB,         // 四8位SRGB颜色（纹理采样时自动转换为线性空间）
    R8G8B8X8_Typeless,     // 四通道8位无类型（X表示未使用）

    // 四通道8位格式 - BGRA顺序
    B8G8R8A8_Typeless,     // 四通道8位无类型（BGR顺序）
    B8G8R8A8_UNorm,        // 四8位无符号归一化（BGR顺序）
    B8G8R8A8_SRGB,         // 四8位SRGB颜色（BGR顺序）
    B8G8R8X8_Typeless,     // 四通道8位无类型（BGR顺序，X表示未使用）

    // 四通道16位格式
    R16G16B16A16_Typeless, // 四通道16位无类型
    R16G16B16A16_UInt,     // 四16位无符号整数
    R16G16B16A16_SInt,     // 四16位有符号整数
    R16G16B16A16_UNorm,    // 四16位无符号归一化
    R16G16B16A16_SNorm,    // 四16位有符号归一化
    R16G16B16A16_Float,    // 四16位浮点数

    // 四通道32位格式
    R32G32B32A32_Typeless, // 四通道32位无类型
    R32G32B32A32_UInt,     // 四32位无符号整数
    R32G32B32A32_SInt,     // 四32位有符号整数
    R32G32B32A32_Float,    // 四32位浮点数

    // 深度/模板格式
    D16_UNorm,             // 16位深度（无模板）- 对应R16_Typeless的深度视图
    D24_UNorm_S8_UInt,     // 24位深度+8位模板
    D24_UNorm_S8_UInt_Typeless, // 24位深度+8位模板无类型
    D32_Float,             // 32位浮点深度（无模板）- 对应R32_Typeless的深度视图
    D32_Float_S8_UInt,     // 32位浮点深度+8位模板
    D32_Float_S8_UInt_Typeless, // 32位浮点深度+8位模板无类型

    // 压缩纹理格式（示例）
    BC1_UNorm,             // BC1压缩（RGB，带透明通道）
    BC2_UNorm,             // BC2压缩（带显式Alpha）
    BC3_UNorm,             // BC3压缩（带插值Alpha）
    BC4_UNorm,             // BC4压缩（单通道）
    BC5_UNorm,             // BC5压缩（双通道）
    BC7_UNorm              // BC7压缩（高质量RGB(A)）
};

// 辅助函数：获取格式的字节大小
// 计算单个数据元素的字节大小（如顶点属性、纹理像素）
inline uint32_t getFormatSize(RALDataFormat format) 
{
    switch (format) 
    {
        // 单通道8位格式（1字节）
    case RALDataFormat::R8_UInt:
    case RALDataFormat::R8_SInt:
    case RALDataFormat::R8_UNorm:
    case RALDataFormat::R8_SNorm:
        return 1;

        // 单通道16位格式（2字节）
    case RALDataFormat::R16_UInt:
    case RALDataFormat::R16_SInt:
    case RALDataFormat::R16_UNorm:
    case RALDataFormat::R16_SNorm:
    case RALDataFormat::R16_Float:
        return 2;

        // 单通道32位格式（4字节）
    case RALDataFormat::R32_UInt:
    case RALDataFormat::R32_SInt:
    case RALDataFormat::R32_Float:
        return 4;

        // 双通道8位格式（2字节 = 8bit * 2）
    case RALDataFormat::R8G8_UInt:
    case RALDataFormat::R8G8_SInt:
    case RALDataFormat::R8G8_UNorm:
    case RALDataFormat::R8G8_SNorm:
        return 2;

        // 双通道16位格式（4字节 = 16bit * 2）
    case RALDataFormat::R16G16_UInt:
    case RALDataFormat::R16G16_SInt:
    case RALDataFormat::R16G16_UNorm:
    case RALDataFormat::R16G16_SNorm:
    case RALDataFormat::R16G16_Float:
        return 4;

        // 双通道32位格式（8字节 = 32bit * 2）
    case RALDataFormat::R32G32_UInt:
    case RALDataFormat::R32G32_SInt:
    case RALDataFormat::R32G32_Float:
        return 8;

        // 三通道8位格式（3字节 = 8bit * 3）
    case RALDataFormat::R8G8B8_UInt:
    case RALDataFormat::R8G8B8_SInt:
    case RALDataFormat::R8G8B8_UNorm:
    case RALDataFormat::R8G8B8_SNorm:
        return 3;

        // 三通道32位格式（12字节 = 32bit * 3）
    case RALDataFormat::R32G32B32_Float:
        return 12;

        // 四通道8位格式（4字节 = 8bit * 4）
    case RALDataFormat::R8G8B8A8_UInt:
    case RALDataFormat::R8G8B8A8_SInt:
    case RALDataFormat::R8G8B8A8_UNorm:
    case RALDataFormat::R8G8B8A8_SNorm:
    case RALDataFormat::R8G8B8A8_SRGB:
        return 4;

        // 四通道16位格式（8字节 = 16bit * 4）
    case RALDataFormat::R16G16B16A16_UInt:
    case RALDataFormat::R16G16B16A16_SInt:
    case RALDataFormat::R16G16B16A16_UNorm:
    case RALDataFormat::R16G16B16A16_SNorm:
    case RALDataFormat::R16G16B16A16_Float:
        return 8;

        // 四通道32位格式（16字节 = 32bit * 4）
    case RALDataFormat::R32G32B32A32_UInt:
    case RALDataFormat::R32G32B32A32_SInt:
    case RALDataFormat::R32G32B32A32_Float:
    case RALDataFormat::R32G32B32A32_Typeless:
        return 16;

        // 三通道32位格式（12字节 = 32bit * 3）
    case RALDataFormat::R32G32B32_Typeless:
        return 12;

        // 四通道16位格式（8字节 = 16bit * 4）
    case RALDataFormat::R16G16B16A16_Typeless:
        return 8;

        // 双通道32位格式（8字节 = 32bit * 2）
    case RALDataFormat::R32G32_Typeless:
        return 8;

        // 双通道16位格式（4字节 = 16bit * 2）
    case RALDataFormat::R16G16_Typeless:
        return 4;

        // 单通道32位格式（4字节）
    case RALDataFormat::R32_Typeless:
        return 4;

        // 单通道16位格式（2字节）
    case RALDataFormat::R16_Typeless:
        return 2;

        // 单通道8位格式（1字节）
    case RALDataFormat::R8_Typeless:
        return 1;

        // 双通道8位格式（2字节 = 8bit * 2）
    case RALDataFormat::R8G8_Typeless:
        return 2;

        // 四通道8位格式（4字节 = 8bit * 4）
    case RALDataFormat::R8G8B8A8_Typeless:
    case RALDataFormat::R8G8B8X8_Typeless:
    case RALDataFormat::B8G8R8A8_Typeless:
    case RALDataFormat::B8G8R8X8_Typeless:
    case RALDataFormat::B8G8R8A8_UNorm:
    case RALDataFormat::B8G8R8A8_SRGB:
        return 4;

        // 深度/模板格式（按实际存储大小）
    case RALDataFormat::D16_UNorm:          // 16位深度（2字节）
        return 2;
    case RALDataFormat::D24_UNorm_S8_UInt:  // 24位深度+8位模板（共4字节，32位对齐）
    case RALDataFormat::D32_Float:          // 32位浮点深度（4字节）
    case RALDataFormat::D32_Float_S8_UInt:  // 32位深度+8位模板（共8字节，64位对齐）
        return format == RALDataFormat::D32_Float_S8_UInt ? 8 : 4;
    case RALDataFormat::D24_UNorm_S8_UInt_Typeless:
        return 4;
    case RALDataFormat::D32_Float_S8_UInt_Typeless:
        return 8;

        // 压缩纹理格式（按4x4像素块的大小计算）
    case RALDataFormat::BC1_UNorm:          // BC1：8字节/4x4块
    case RALDataFormat::BC4_UNorm:          // BC4：8字节/4x4块
        return 8;
    case RALDataFormat::BC2_UNorm:          // BC2：16字节/4x4块
    case RALDataFormat::BC3_UNorm:          // BC3：16字节/4x4块
    case RALDataFormat::BC5_UNorm:          // BC5：16字节/4x4块
    case RALDataFormat::BC7_UNorm:          // BC7：16字节/4x4块
        return 16;

        // 未定义格式（错误情况）
    case RALDataFormat::Undefined:
    default:
        return 0;
    }
}

// 辅助函数：获取对应的无类型格式
inline RALDataFormat getTypelessFormat(RALDataFormat format)
{
    switch (format)
    {
        // R32G32B32A32系列
    case RALDataFormat::R32G32B32A32_UInt:
    case RALDataFormat::R32G32B32A32_SInt:
    case RALDataFormat::R32G32B32A32_Float:
        return RALDataFormat::R32G32B32A32_Typeless;

        // R32G32B32系列
    case RALDataFormat::R32G32B32_Float:
        return RALDataFormat::R32G32B32_Typeless;

        // R16G16B16A16系列
    case RALDataFormat::R16G16B16A16_UInt:
    case RALDataFormat::R16G16B16A16_SInt:
    case RALDataFormat::R16G16B16A16_UNorm:
    case RALDataFormat::R16G16B16A16_SNorm:
    case RALDataFormat::R16G16B16A16_Float:
        return RALDataFormat::R16G16B16A16_Typeless;

        // R32G32系列
    case RALDataFormat::R32G32_UInt:
    case RALDataFormat::R32G32_SInt:
    case RALDataFormat::R32G32_Float:
        return RALDataFormat::R32G32_Typeless;

        // R16G16系列
    case RALDataFormat::R16G16_UInt:
    case RALDataFormat::R16G16_SInt:
    case RALDataFormat::R16G16_UNorm:
    case RALDataFormat::R16G16_SNorm:
    case RALDataFormat::R16G16_Float:
        return RALDataFormat::R16G16_Typeless;

        // R32系列
    case RALDataFormat::R32_UInt:
    case RALDataFormat::R32_SInt:
    case RALDataFormat::R32_Float:
    case RALDataFormat::D32_Float:
        return RALDataFormat::R32_Typeless;

        // R16系列
    case RALDataFormat::R16_UInt:
    case RALDataFormat::R16_SInt:
    case RALDataFormat::R16_UNorm:
    case RALDataFormat::R16_SNorm:
    case RALDataFormat::R16_Float:
    case RALDataFormat::D16_UNorm:
        return RALDataFormat::R16_Typeless;

        // R8系列
    case RALDataFormat::R8_UInt:
    case RALDataFormat::R8_SInt:
    case RALDataFormat::R8_UNorm:
    case RALDataFormat::R8_SNorm:
        return RALDataFormat::R8_Typeless;

        // R8G8系列
    case RALDataFormat::R8G8_UInt:
    case RALDataFormat::R8G8_SInt:
    case RALDataFormat::R8G8_UNorm:
    case RALDataFormat::R8G8_SNorm:
        return RALDataFormat::R8G8_Typeless;

        // R8G8B8A8系列
    case RALDataFormat::R8G8B8A8_UInt:
    case RALDataFormat::R8G8B8A8_SInt:
    case RALDataFormat::R8G8B8A8_UNorm:
    case RALDataFormat::R8G8B8A8_SNorm:
    case RALDataFormat::R8G8B8A8_SRGB:
        return RALDataFormat::R8G8B8A8_Typeless;

        // B8G8R8A8系列
    case RALDataFormat::B8G8R8A8_UNorm:
    case RALDataFormat::B8G8R8A8_SRGB:
        return RALDataFormat::B8G8R8A8_Typeless;

        // 深度模板格式
    case RALDataFormat::D24_UNorm_S8_UInt:
        return RALDataFormat::D24_UNorm_S8_UInt_Typeless;
    case RALDataFormat::D32_Float_S8_UInt:
        return RALDataFormat::D32_Float_S8_UInt_Typeless;

    default:
        return format; // 如果找不到对应的无类型格式，返回原格式
    }
}

// 辅助函数：从无类型格式获取浮点类型
inline RALDataFormat getFloatFormatFromTypeless(RALDataFormat typelessFormat)
{
    switch (typelessFormat)
    {
    case RALDataFormat::R32G32B32A32_Typeless:
        return RALDataFormat::R32G32B32A32_Float;
    case RALDataFormat::R32G32B32_Typeless:
        return RALDataFormat::R32G32B32_Float;
    case RALDataFormat::R16G16B16A16_Typeless:
        return RALDataFormat::R16G16B16A16_Float;
    case RALDataFormat::R32G32_Typeless:
        return RALDataFormat::R32G32_Float;
    case RALDataFormat::R16G16_Typeless:
        return RALDataFormat::R16G16_Float;
    case RALDataFormat::R32_Typeless:
        return RALDataFormat::R32_Float;
    case RALDataFormat::R16_Typeless:
        return RALDataFormat::R16_Float;
    default:
        return RALDataFormat::Undefined; // 不支持的转换
    }
}

// 辅助函数：从无类型格式获取UNorm类型
inline RALDataFormat getUNormFormatFromTypeless(RALDataFormat typelessFormat)
{
    switch (typelessFormat)
    {
    case RALDataFormat::R8G8B8A8_Typeless:
        return RALDataFormat::R8G8B8A8_UNorm;
    case RALDataFormat::B8G8R8A8_Typeless:
        return RALDataFormat::B8G8R8A8_UNorm;
    case RALDataFormat::R16G16B16A16_Typeless:
        return RALDataFormat::R16G16B16A16_UNorm;
    case RALDataFormat::R16_Typeless:
        return RALDataFormat::R16_UNorm;
    case RALDataFormat::R8_Typeless:
        return RALDataFormat::R8_UNorm;
    case RALDataFormat::R8G8_Typeless:
        return RALDataFormat::R8G8_UNorm;
    case RALDataFormat::R16G16_Typeless:
        return RALDataFormat::R16G16_UNorm;
    case RALDataFormat::R32_Typeless:
        return RALDataFormat::Undefined; // 注意：32位通常不用于UNorm格式，但为完整性添加此case
    default:
        return RALDataFormat::Undefined; // 不支持的转换
    }
}

// 辅助函数：从无类型格式获取深度模板类型
inline RALDataFormat getDepthStencilFormatFromTypeless(RALDataFormat typelessFormat)
{
    switch (typelessFormat)
    {
    case RALDataFormat::R32_Typeless:
        return RALDataFormat::D32_Float;
    case RALDataFormat::R16_Typeless:
        return RALDataFormat::D16_UNorm;
    case RALDataFormat::D24_UNorm_S8_UInt_Typeless:
        return RALDataFormat::D24_UNorm_S8_UInt;
    case RALDataFormat::D32_Float_S8_UInt_Typeless:
        return RALDataFormat::D32_Float_S8_UInt;
    default:
        return RALDataFormat::Undefined; // 不支持的转换
    }
}

// 辅助函数：检查格式是否为无类型格式
inline bool isTypelessFormat(RALDataFormat format)
{
    switch (format)
    {
    case RALDataFormat::R32G32B32A32_Typeless:
    case RALDataFormat::R32G32B32_Typeless:
    case RALDataFormat::R16G16B16A16_Typeless:
    case RALDataFormat::R32G32_Typeless:
    case RALDataFormat::R16G16_Typeless:
    case RALDataFormat::R32_Typeless:
    case RALDataFormat::R16_Typeless:
    case RALDataFormat::R8_Typeless:
    case RALDataFormat::R8G8_Typeless:
    case RALDataFormat::R8G8B8A8_Typeless:
    case RALDataFormat::R8G8B8X8_Typeless:
    case RALDataFormat::B8G8R8A8_Typeless:
    case RALDataFormat::B8G8R8X8_Typeless:
    case RALDataFormat::D24_UNorm_S8_UInt_Typeless:
    case RALDataFormat::D32_Float_S8_UInt_Typeless:
        return true;
    default:
        return false;
    }
}

#endif// RAL_DATA_FORMAT_H