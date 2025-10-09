cmake_minimum_required(VERSION 3.16)
project(ClothSimulator)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加UTF-8编码支持
add_compile_options("/source-charset:utf-8" "/execution-charset:utf-8")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Release_SolverDebug;Debug_SolverDebug" CACHE STRING "Available build types" FORCE)

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

# 为所有构建类型添加调试信息
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # 为Debug版本添加调试信息和宏定义
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/Zi /Od /DDEBUG=1>
        # 为Release版本添加调试信息但保持优化
        $<$<CONFIG:Release>:/Zi /O2 /Oy- /Gw /GS /GL>
        # 为Release_SolverDebug版本添加调试信息、保持优化并开启DEBUG_SOLVER宏
        $<$<CONFIG:Release_SolverDebug>:/Zi /O2 /Oy- /Gw /GS /GL /DDEBUG_SOLVER=1>
        # 为Debug_SolverDebug版本添加调试信息、禁用优化并开启DEBUG和DEBUG_SOLVER宏
        $<$<CONFIG:Debug_SolverDebug>:/Zi /Od /DDEBUG=1 /DDEBUG_SOLVER=1>
    )
    
    # 为链接器添加调试相关选项
    target_link_options(${PROJECT_NAME} PRIVATE
        # 为Debug版本添加链接器调试选项
        $<$<CONFIG:Debug>:/DEBUG:FULL /INCREMENTAL:NO>
        # 为Release版本添加链接器调试选项，确保生成完整的调试信息，并添加/LTCG以匹配/GL编译选项
        $<$<CONFIG:Release>:/DEBUG:FULL /OPT:REF /OPT:ICF /INCREMENTAL:NO /LTCG>
        # 为Release_SolverDebug版本添加与Release相同的链接器调试选项，并添加/LTCG以匹配/GL编译选项
        $<$<CONFIG:Release_SolverDebug>:/DEBUG:FULL /OPT:REF /OPT:ICF /INCREMENTAL:NO /LTCG>
        # 为Debug_SolverDebug版本添加与Debug相同的链接器调试选项
        $<$<CONFIG:Debug_SolverDebug>:/DEBUG:FULL /INCREMENTAL:NO>
    )
    
    # 确保程序数据库文件(PDB)生成
    set_target_properties(${PROJECT_NAME} PROPERTIES
        # 编译PDB配置
        COMPILE_PDB_NAME "${PROJECT_NAME}"
        COMPILE_PDB_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        
        # 确保调试信息和可执行文件在同一目录
        PDB_NAME "${PROJECT_NAME}"
        PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        
        # 确保符号信息完整
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endif()