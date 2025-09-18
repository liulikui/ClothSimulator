#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include "Camera.h"
#include "RALResource.h"
#include "DX12RALResource.h"
#include "TSharePtr.h"

// 简化命名空间
namespace dx = DirectX;

// 前向声明
class Scene;

// 交换链常量缓冲区结构体
struct ConstantBuffer {
    dx::XMFLOAT4X4 worldViewProj; // 世界-视图-投影矩阵
    dx::XMFLOAT4X4 world;         // 世界矩阵
};

// 材质和光照常量缓冲区结构体
struct MaterialBuffer {
    dx::XMFLOAT4 diffuseColor;    // 漫反射颜色
    dx::XMFLOAT3 lightPos;        // 光源位置
    float padding1;               // 4字节对齐填充
    dx::XMFLOAT3 lightColor;      // 光源颜色
    float padding2;               // 4字节对齐填充
};

class DX12Renderer {
public:
    // 构造函数和析构函数
    DX12Renderer(uint32_t width, uint32_t height, const std::wstring& windowName, HWND hWnd);
    ~DX12Renderer();

    // 初始化DirectX 12
    bool Initialize();

    // 渲染一帧
    void Render(Scene* scene, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);

    // 清理资源
    void Cleanup();

    // 设置布料顶点数据
    void SetClothVertices(const std::vector<dx::XMFLOAT3>& positions, const std::vector<dx::XMFLOAT3>& normals,
                         const std::vector<uint32_t>& indices);

    // 设置球体顶点数据
    void SetSphereVertices(const std::vector<dx::XMFLOAT3>& positions, const std::vector<dx::XMFLOAT3>& normals,
                          const std::vector<uint32_t>& indices);

    // 调整窗口大小
    void Resize(uint32_t width, uint32_t height);
    
    // 更新常量缓冲区
    bool CreateMaterialBuffer();
    void UpdateMaterialBuffer(const dx::XMFLOAT4& diffuseColor);

    // 更新光源位置
    void UpdateLightPosition(const dx::XMFLOAT4& position);
    
    // 更新光源颜色
    void UpdateLightColor(const dx::XMFLOAT4& color);

    // 获取窗口宽度
    uint32_t GetWidth() const { return m_width; }

    // 获取窗口高度
    uint32_t GetHeight() const { return m_height; }
    
    // 渲染单个Primitive对象
    // 参数：
    //   worldMatrix - 世界矩阵
    //   diffuseColor - 漫反射颜色
    //   isCloth - 是否为布料对象（使用布料的PSO）
    // 注意：视图矩阵和投影矩阵不再作为参数传递，因为它们已经存储在Scene的cameraConstBuffer中
    void RenderPrimitive(const dx::XMMATRIX& worldMatrix, const dx::XMFLOAT4& diffuseColor, bool isCloth);
    
    // 设置根签名
    void SetRootSignature(std::unique_ptr<IRALRootSignature> rootSignature);
    
    // 创建并获取根签名
    std::unique_ptr<IRALRootSignature> CreateAndGetRootSignature();
    
    // 编译顶点着色器
    std::unique_ptr<IRALVertexShader> CompileVertexShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译像素着色器
    std::unique_ptr<IRALPixelShader> CompilePixelShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译几何着色器
    std::unique_ptr<IRALGeometryShader> CompileGeometryShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译计算着色器
    std::unique_ptr<IRALComputeShader> CompileComputeShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译网格着色器
    std::unique_ptr<IRALMeshShader> CompileMeshShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译放大着色器
    std::unique_ptr<IRALAmplificationShader> CompileAmplificationShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译光线生成着色器
    std::unique_ptr<IRALRayGenShader> CompileRayGenShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译光线未命中着色器
    std::unique_ptr<IRALRayMissShader> CompileRayMissShader(const char* shaderCode, const char* entryPoint = "main");
    
    // 编译光线命中组着色器
	std::unique_ptr<IRALRayHitGroupShader> CompileRayHitGroupShader(const char* shaderCode, const char* entryPoint = "main");
	
	// 编译光线可调用着色器
	std::unique_ptr<IRALRayCallableShader> CompileRayCallableShader(const char* shaderCode, const char* entryPoint = "main");
	
	// 创建图形管线状态
	std::unique_ptr<IGraphicsPipelineState> CreateGraphicsPipelineState(const RALGraphicsPipelineStateDesc& desc);

private:
    // 通用着色器编译辅助方法
    Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderBlob(const char* shaderCode, const char* entryPoint, const char* target);
    
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

    // 创建根签名
    void CreateRootSignature();

    // 创建管道状态对象
    void CreatePipelineStateObjects();

    // 创建缓冲区
    TSharePtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
                                             D3D12_HEAP_PROPERTIES heapProps,
                                             D3D12_RESOURCE_STATES initialState);

    // 上传资源数据
    template<typename T>
    void UploadBufferData(TSharePtr<ID3D12Resource>& buffer, const std::vector<T>& data);

    // 等待前一帧完成
    void WaitForPreviousFrame();

    // 成员变量
    uint32_t m_width;                           // 窗口宽度
    uint32_t m_height;                          // 窗口高度
    std::wstring m_windowName;              // 窗口名称
    HWND m_hWnd;                            // 窗口句柄

    // 设备和交换链
    TSharePtr<ID3D12Device> m_device;                    // D3D12设备
    
    // 根签名
    std::unique_ptr<IRALRootSignature> m_rootSignature;
    TSharePtr<IDXGIFactory6> m_factory;                  // DXGI工厂
    TSharePtr<IDXGISwapChain4> m_swapChain;              // 交换链
    uint32_t m_backBufferCount = 2;                            // 后缓冲区数量
    uint32_t m_currentBackBufferIndex = 0;                     // 当前后缓冲区索引

    // 命令对象
    TSharePtr<ID3D12CommandAllocator> m_commandAllocators[2];    // 命令分配器数组
    TSharePtr<ID3D12GraphicsCommandList> m_commandList;      // 命令列表
    TSharePtr<ID3D12CommandQueue> m_commandQueue;            // 命令队列

    // 同步对象
    TSharePtr<ID3D12Fence> m_fence;                     // 围栏
    uint64_t m_fenceValue = 0;                              // 围栏值
    HANDLE m_fenceEvent = nullptr;                        // 围栏事件
    uint32_t m_currentFrameIndex; // 当前帧索引，用于缓存

    // 描述符堆
    TSharePtr<ID3D12DescriptorHeap> m_rtvHeap;          // 渲染目标视图堆
    TSharePtr<ID3D12DescriptorHeap> m_dsvHeap;          // 深度/模板视图堆
    TSharePtr<ID3D12DescriptorHeap> m_srvHeap;          // 着色器资源视图堆
    uint32_t m_rtvDescriptorSize = 0;                         // 渲染目标视图描述符大小
    uint32_t m_dsvDescriptorSize = 0;                         // 深度/模板视图描述符大小
    uint32_t m_srvDescriptorSize = 0;                         // 着色器资源视图描述符大小

    // 资源
    std::vector<TSharePtr<ID3D12Resource>> m_backBuffers;       // 后缓冲区
    TSharePtr<ID3D12Resource> m_depthStencilBuffer;             // 深度/模板缓冲区

    // 根签名和管道状态
    TSharePtr<ID3D12PipelineState> m_clothPipelineState;        // 布料管道状态
    TSharePtr<ID3D12PipelineState> m_spherePipelineState;       // 球体管道状态

    // 布料和球体资源
    TSharePtr<ID3D12Resource> m_clothVertexBuffer;              // 布料顶点缓冲区
    TSharePtr<ID3D12Resource> m_clothIndexBuffer;               // 布料索引缓冲区
    TSharePtr<ID3D12Resource> m_sphereVertexBuffer;             // 球体顶点缓冲区
    TSharePtr<ID3D12Resource> m_sphereIndexBuffer;              // 球体索引缓冲区

    uint32_t m_clothVertexCount = 0;                                  // 布料顶点数量
    uint32_t m_clothIndexCount = 0;                                   // 布料索引数量
    uint32_t m_sphereVertexCount = 0;                                 // 球体顶点数量
    uint32_t m_sphereIndexCount = 0;                                  // 球体索引数量
    
    // 常量缓冲区
    TSharePtr<ID3D12Resource> m_materialBuffer;                 // 材质和光照常量缓冲区

    // 光照参数
    dx::XMFLOAT4 m_lightPosition = { -10.0f, 30.0f, -10.0f, 1.0f };
    dx::XMFLOAT4 m_lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};
