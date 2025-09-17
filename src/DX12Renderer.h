#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <memory>
#include "Camera.h"

// 前向声明
class Scene;

// 简化命名空间
namespace dx = DirectX;
namespace wrl = Microsoft::WRL;

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
    void Render(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);

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
    bool CreateConstantBuffers();
    void UpdateConstantBuffer(const dx::XMMATRIX& worldMatrix, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix);
    void UpdateMaterialBuffer(const dx::XMFLOAT4& diffuseColor);

    // 更新光源位置
    void UpdateLightPosition(const dx::XMFLOAT4& position);
    
    // 更新光源颜色
    void UpdateLightColor(const dx::XMFLOAT4& color);

    // 获取窗口宽度
    uint32_t GetWidth() const { return width_; }

    // 获取窗口高度
    uint32_t GetHeight() const { return height_; }

private:
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
    wrl::ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
                                             D3D12_HEAP_PROPERTIES heapProps,
                                             D3D12_RESOURCE_STATES initialState);

    // 上传资源数据
    template<typename T>
    void UploadBufferData(wrl::ComPtr<ID3D12Resource>& buffer, const std::vector<T>& data);

    // 等待前一帧完成
    void WaitForPreviousFrame();

    // 成员变量
    uint32_t width_;                           // 窗口宽度
    uint32_t height_;                          // 窗口高度
    std::wstring windowName_;              // 窗口名称
    HWND hWnd_;                            // 窗口句柄

    // 设备和交换链
    wrl::ComPtr<ID3D12Device> device_;                    // D3D12设备
    wrl::ComPtr<IDXGIFactory6> factory_;                  // DXGI工厂
    wrl::ComPtr<IDXGISwapChain4> swapChain_;              // 交换链
    uint32_t backBufferCount_ = 2;                            // 后缓冲区数量
    uint32_t currentBackBufferIndex_ = 0;                     // 当前后缓冲区索引

    // 命令对象
    wrl::ComPtr<ID3D12CommandAllocator> commandAllocators_[2];    // 命令分配器数组
    wrl::ComPtr<ID3D12GraphicsCommandList> commandList_;      // 命令列表
    wrl::ComPtr<ID3D12CommandQueue> commandQueue_;            // 命令队列

    // 同步对象
    wrl::ComPtr<ID3D12Fence> fence_;                     // 围栏
    uint64_t fenceValue_ = 0;                              // 围栏值
    HANDLE fenceEvent_ = nullptr;                        // 围栏事件
    uint32_t currentFrameIndex_; // 当前帧索引，用于缓存

    // 描述符堆
    wrl::ComPtr<ID3D12DescriptorHeap> rtvHeap_;          // 渲染目标视图堆
    wrl::ComPtr<ID3D12DescriptorHeap> dsvHeap_;          // 深度/模板视图堆
    wrl::ComPtr<ID3D12DescriptorHeap> srvHeap_;          // 着色器资源视图堆
    uint32_t rtvDescriptorSize_ = 0;                         // 渲染目标视图描述符大小
    uint32_t dsvDescriptorSize_ = 0;                         // 深度/模板视图描述符大小
    uint32_t srvDescriptorSize_ = 0;                         // 着色器资源视图描述符大小

    // 资源
    std::vector<wrl::ComPtr<ID3D12Resource>> backBuffers_;       // 后缓冲区
    wrl::ComPtr<ID3D12Resource> depthStencilBuffer_;             // 深度/模板缓冲区

    // 根签名和管道状态
    wrl::ComPtr<ID3D12RootSignature> rootSignature_;             // 根签名
    wrl::ComPtr<ID3D12PipelineState> clothPipelineState_;        // 布料管道状态
    wrl::ComPtr<ID3D12PipelineState> spherePipelineState_;       // 球体管道状态

    // 布料和球体资源
    wrl::ComPtr<ID3D12Resource> clothVertexBuffer_;              // 布料顶点缓冲区
    wrl::ComPtr<ID3D12Resource> clothIndexBuffer_;               // 布料索引缓冲区
    wrl::ComPtr<ID3D12Resource> sphereVertexBuffer_;             // 球体顶点缓冲区
    wrl::ComPtr<ID3D12Resource> sphereIndexBuffer_;              // 球体索引缓冲区

    uint32_t clothVertexCount_ = 0;                                  // 布料顶点数量
    uint32_t clothIndexCount_ = 0;                                   // 布料索引数量
    uint32_t sphereVertexCount_ = 0;                                 // 球体顶点数量
    uint32_t sphereIndexCount_ = 0;                                  // 球体索引数量
    
    // 常量缓冲区
    wrl::ComPtr<ID3D12Resource> constantBuffer_;                 // 交换矩阵常量缓冲区
    wrl::ComPtr<ID3D12Resource> materialBuffer_;                 // 材质和光照常量缓冲区

    // 光照参数
    dx::XMFLOAT4 lightPosition_ = { -10.0f, 30.0f, -10.0f, 1.0f };
    dx::XMFLOAT4 lightColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
};
