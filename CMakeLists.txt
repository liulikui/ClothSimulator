cmake_minimum_required(VERSION 3.16)
project(XPBDClothSimulator)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加UTF-8编码支持
add_compile_options("/source-charset:utf-8" "/execution-charset:utf-8")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# 添加DirectX 12和Windows SDK的包含路径
include_directories($ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/um)
include_directories($ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared)
include_directories($ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/winrt)

# 添加源代码目录
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接DirectX 12和Windows库
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    D3DCompiler.lib
    User32.lib
    Kernel32.lib
    Gdi32.lib
)

# 定义预处理器宏
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
)

# 设置Windows子系统（GUI应用）
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE YES
)

# 为Debug版本添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(XPBDClothSimulator_DX12 PRIVATE DEBUG=1)
endif()