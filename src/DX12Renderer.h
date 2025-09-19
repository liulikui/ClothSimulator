#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include "Camera.h"
#include "RALResource.h"
#include "RALCommandList.h"
#include "DX12RALResource.h"
#include "TSharePtr.h"

// 简化命名空间
namespace dx = DirectX;

// 前向声明
class Scene;

class DX12Renderer
{
public:
    // 构造函数和析构函数
    DX12Renderer(uint32_t width, uint32_t height, const std::wstring& windowName, HWND hWnd);
    ~DX12Renderer();

    // 初始化DirectX 12
    bool Initialize();

    // 开始一帧
    void BeginFrame(IRALGraphicsCommandList* commandList);

    // 提交CommandLists
    void ExecuteCommandLists(uint32_t count, IRALCommandList** ppCommandList);

    // 结束一帧
    void EndFrame();

    // 清理资源
    void Cleanup();

    // 调整窗口大小
    void Resize(uint32_t width, uint32_t height);

    // 获取窗口宽度
    uint32_t GetWidth() const { return m_width; }

    // 获取窗口高度
    uint32_t GetHeight() const { return m_height; }
    
    // 编译顶点着色器
    IRALVertexShader* CompileVertexShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译像素着色器
    IRALPixelShader* CompilePixelShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译几何着色器
    IRALGeometryShader* CompileGeometryShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译计算着色器
    IRALComputeShader* CompileComputeShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译网格着色器
    IRALMeshShader* CompileMeshShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译放大着色器
    IRALAmplificationShader* CompileAmplificationShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译光线生成着色器
    IRALRayGenShader* CompileRayGenShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译光线未命中着色器
    IRALRayMissShader* CompileRayMissShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译光线命中组着色器
	IRALRayHitGroupShader* CompileRayHitGroupShader(const char* shaderCode, const char* entryPoint = "main");
	
	// 编译光线可调用着色器
	IRALRayCallableShader* CompileRayCallableShader(const char* shaderCode, const char* entryPoint = "main");
	
	// 创建图形管线状态
	IRALGraphicsPipelineState* CreateGraphicsPipelineState(const RALGraphicsPipelineStateDesc& desc);

    // 创建根签名 
    IRALRootSignature* CreateRootSignature(const std::vector<RALRootParameter>& rootParameters,
        const std::vector<RALStaticSampler>& staticSamplers = {},
        RALRootSignatureFlags flags = RALRootSignatureFlags::AllowInputAssemblerInputLayout);
        
    // 创建图形命令列表
    IRALGraphicsCommandList* CreateGraphicsCommandList();

    // 创建顶点缓冲区
    IRALVertexBuffer* CreateVertexBuffer(uint64_t size, uint32_t stride, bool isStatic = true);

    // 创建索引缓冲区
    IRALIndexBuffer* CreateIndexBuffer(uint64_t size, bool is32BitIndex = true, bool isStatic = true);

    // 创建常量缓冲区
    IRALConstBuffer* CreateConstBuffer(uint64_t size);

    // 更新Buffer
    bool UploadBuffer(IRALBuffer* buffer, const char* data, uint64_t size);

private:
    // 通用着色器编译辅助方法
    ComPtr<ID3DBlob> CompileShaderBlob(const char* shaderCode, const char* entryPoint, const char* target);
    
    // 创建设备和交换链
    bool CreateDeviceAndSwapChain();

    // 创建命令对象
    void CreateCommandObjects();

    // 创建描述符堆
    void CreateDescriptorHeaps();

    // 创建渲染目标视图
    void CreateRenderTargetViews();

    // 创建深度/模板视图
    void CreateDepthStencilView();

    // 创建缓冲区
    ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
        D3D12_HEAP_PROPERTIES heapProps,
        D3D12_RESOURCE_STATES initialState);

    // 等待前一帧完成
    void WaitForPreviousFrame();

    // 成员变量
    uint32_t m_width;                           // 窗口宽度
    uint32_t m_height;                          // 窗口高度
    std::wstring m_windowName;              // 窗口名称
    HWND m_hWnd;                            // 窗口句柄

    // 设备和交换链
    ComPtr<ID3D12Device> m_device;                    // D3D12设备
    
    // 根签名
    TSharePtr<IRALRootSignature> m_rootSignature;
    ComPtr<IDXGIFactory6> m_factory;                  // DXGI工厂
    ComPtr<IDXGISwapChain4> m_swapChain;              // 交换链
    uint32_t m_backBufferCount = 2;                            // 后缓冲区数量
    uint32_t m_currentBackBufferIndex = 0;                     // 当前后缓冲区索引

    // 命令对象
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[2];    // 命令分配器数组
    ComPtr<ID3D12GraphicsCommandList> m_commandList;      // 命令列表
    ComPtr<ID3D12CommandQueue> m_commandQueue;            // 命令队列

    // 同步对象
    ComPtr<ID3D12Fence> m_fence;                     // 围栏
    uint64_t m_fenceValue = 0;                              // 围栏值
    HANDLE m_fenceEvent = nullptr;                        // 围栏事件
    uint32_t m_currentFrameIndex; // 当前帧索引，用于缓存

    // 描述符堆
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;          // 渲染目标视图堆
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;          // 深度/模板视图堆
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;          // 着色器资源视图堆
    uint32_t m_rtvDescriptorSize = 0;                         // 渲染目标视图描述符大小
    uint32_t m_dsvDescriptorSize = 0;                         // 深度/模板视图描述符大小
    uint32_t m_srvDescriptorSize = 0;                         // 着色器资源视图描述符大小

    // 资源
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;       // 后缓冲区
    ComPtr<ID3D12Resource> m_depthStencilBuffer;             // 深度/模板缓冲区
};
