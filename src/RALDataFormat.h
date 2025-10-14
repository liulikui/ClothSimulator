#ifndef RAL_DATA_FORMAT_H
#define RAL_DATA_FORMAT_H

#include <cstdint>

// 跨平台数据格式枚举（覆盖顶点属性、纹理、缓冲区等通用格式）
enum class DataFormat : uint32_t 
{
    // 未定义格式
    Undefined,

    // 8位整数格式
    R8_UInt,          // 单通道8位无符号整数
    R8_SInt,          // 单通道8位有符号整数
    R8_UNorm,         // 单通道8位无符号归一化（[0,1]）
    R8_SNorm,         // 单通道8位有符号归一化（[-1,1]）

    // 16位整数格式
    R16_UInt,         // 单通道16位无符号整数
    R16_SInt,         // 单通道16位有符号整数
    R16_UNorm,        // 单通道16位无符号归一化
    R16_SNorm,        // 单通道16位有符号归一化
    R16_Float,        // 单通道16位浮点数（半精度）

    // 32位整数/浮点数格式
    R32_UInt,         // 单通道32位无符号整数
    R32_SInt,         // 单通道32位有符号整数
    R32_Float,        // 单通道32位浮点数（单精度）

    // 双通道8位格式
    R8G8_UInt,        // 双8位无符号整数
    R8G8_SInt,        // 双8位有符号整数
    R8G8_UNorm,       // 双8位无符号归一化
    R8G8_SNorm,       // 双8位有符号归一化

    // 双通道16位格式
    R16G16_UInt,      // 双16位无符号整数
    R16G16_SInt,      // 双16位有符号整数
    R16G16_UNorm,     // 双16位无符号归一化
    R16G16_SNorm,     // 双16位有符号归一化
    R16G16_Float,     // 双16位浮点数

    // 双通道32位格式
    R32G32_UInt,      // 双32位无符号整数
    R32G32_SInt,      // 双32位有符号整数
    R32G32_Float,     // 双32位浮点数

    // 三通道格式（注意：某些API对3通道格式支持有限）
    R8G8B8_UInt,      // 三8位无符号整数
    R8G8B8_SInt,      // 三8位有符号整数
    R8G8B8_UNorm,     // 三8位无符号归一化
    R8G8B8_SNorm,     // 三8位有符号归一化
    R32G32B32_Float,  // 三32位浮点数

    // 四通道8位格式
    R8G8B8A8_UInt,    // 四8位无符号整数
    R8G8B8A8_SInt,    // 四8位有符号整数
    R8G8B8A8_UNorm,   // 四8位无符号归一化（常用RGBA颜色）
    R8G8B8A8_SNorm,   // 四8位有符号归一化
    R8G8B8A8_SRGB,    // 四8位SRGB颜色（纹理采样时自动转换为线性空间）

    // 四通道16位格式
    R16G16B16A16_UInt,    // 四16位无符号整数
    R16G16B16A16_SInt,    // 四16位有符号整数
    R16G16B16A16_UNorm,   // 四16位无符号归一化
    R16G16B16A16_SNorm,   // 四16位有符号归一化
    R16G16B16A16_Float,   // 四16位浮点数

    // 四通道32位格式
    R32G32B32A32_UInt,    // 四32位无符号整数
    R32G32B32A32_SInt,    // 四32位有符号整数
    R32G32B32A32_Float,   // 四32位浮点数

    // 深度/模板格式
    D16_UNorm,        // 16位深度（无模板）
    D24_UNorm_S8_UInt,// 24位深度+8位模板
    D32_Float,        // 32位浮点深度（无模板）
    D32_Float_S8_UInt,// 32位浮点深度+8位模板

    // 压缩纹理格式（示例）
    BC1_UNorm,        // BC1压缩（RGB，带透明通道）
    BC2_UNorm,        // BC2压缩（带显式Alpha）
    BC3_UNorm,        // BC3压缩（带插值Alpha）
    BC4_UNorm,        // BC4压缩（单通道）
    BC5_UNorm,        // BC5压缩（双通道）
    BC7_UNorm,        // BC7压缩（高质量RGB(A)）
};

// 辅助函数：获取格式的字节大小
// 计算单个数据元素的字节大小（如顶点属性、纹理像素）
inline uint32_t getFormatSize(DataFormat format) 
{
    switch (format) 
    {
        // 单通道8位格式（1字节）
    case DataFormat::R8_UInt:
    case DataFormat::R8_SInt:
    case DataFormat::R8_UNorm:
    case DataFormat::R8_SNorm:
        return 1;

        // 单通道16位格式（2字节）
    case DataFormat::R16_UInt:
    case DataFormat::R16_SInt:
    case DataFormat::R16_UNorm:
    case DataFormat::R16_SNorm:
    case DataFormat::R16_Float:
        return 2;

        // 单通道32位格式（4字节）
    case DataFormat::R32_UInt:
    case DataFormat::R32_SInt:
    case DataFormat::R32_Float:
        return 4;

        // 双通道8位格式（2字节 = 8bit * 2）
    case DataFormat::R8G8_UInt:
    case DataFormat::R8G8_SInt:
    case DataFormat::R8G8_UNorm:
    case DataFormat::R8G8_SNorm:
        return 2;

        // 双通道16位格式（4字节 = 16bit * 2）
    case DataFormat::R16G16_UInt:
    case DataFormat::R16G16_SInt:
    case DataFormat::R16G16_UNorm:
    case DataFormat::R16G16_SNorm:
    case DataFormat::R16G16_Float:
        return 4;

        // 双通道32位格式（8字节 = 32bit * 2）
    case DataFormat::R32G32_UInt:
    case DataFormat::R32G32_SInt:
    case DataFormat::R32G32_Float:
        return 8;

        // 三通道8位格式（3字节 = 8bit * 3）
    case DataFormat::R8G8B8_UInt:
    case DataFormat::R8G8B8_SInt:
    case DataFormat::R8G8B8_UNorm:
    case DataFormat::R8G8B8_SNorm:
        return 3;

        // 三通道32位格式（12字节 = 32bit * 3）
    case DataFormat::R32G32B32_Float:
        return 12;

        // 四通道8位格式（4字节 = 8bit * 4）
    case DataFormat::R8G8B8A8_UInt:
    case DataFormat::R8G8B8A8_SInt:
    case DataFormat::R8G8B8A8_UNorm:
    case DataFormat::R8G8B8A8_SNorm:
    case DataFormat::R8G8B8A8_SRGB:
        return 4;

        // 四通道16位格式（8字节 = 16bit * 4）
    case DataFormat::R16G16B16A16_UInt:
    case DataFormat::R16G16B16A16_SInt:
    case DataFormat::R16G16B16A16_UNorm:
    case DataFormat::R16G16B16A16_SNorm:
    case DataFormat::R16G16B16A16_Float:
        return 8;

        // 四通道32位格式（16字节 = 32bit * 4）
    case DataFormat::R32G32B32A32_UInt:
    case DataFormat::R32G32B32A32_SInt:
    case DataFormat::R32G32B32A32_Float:
        return 16;

        // 深度/模板格式（按实际存储大小）
    case DataFormat::D16_UNorm:          // 16位深度（2字节）
        return 2;
    case DataFormat::D24_UNorm_S8_UInt:  // 24位深度+8位模板（共4字节，32位对齐）
    case DataFormat::D32_Float:          // 32位浮点深度（4字节）
    case DataFormat::D32_Float_S8_UInt:  // 32位深度+8位模板（共8字节，64位对齐）
        return format == DataFormat::D32_Float_S8_UInt ? 8 : 4;

        // 压缩纹理格式（按4x4像素块的大小计算）
    case DataFormat::BC1_UNorm:          // BC1：8字节/4x4块
    case DataFormat::BC4_UNorm:          // BC4：8字节/4x4块
        return 8;
    case DataFormat::BC2_UNorm:          // BC2：16字节/4x4块
    case DataFormat::BC3_UNorm:          // BC3：16字节/4x4块
    case DataFormat::BC5_UNorm:          // BC5：16字节/4x4块
    case DataFormat::BC7_UNorm:          // BC7：16字节/4x4块
        return 16;

        // 未定义格式（错误情况）
    case DataFormat::Undefined:
    default:
        return 0;
    }
}

#endif// RAL_DATA_FORMAT_H