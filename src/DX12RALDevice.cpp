#include "DX12RALDevice.h"
#include "Scene.h"
#include "TRefCountPtr.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <d3dcompiler.h>
#ifdef _DEBUG
#include <d3d12sdklayers.h> // 包含D3D12调试层相关的定义
#endif

#include "DX12RALResource.h"
#include "DX12RALCommandList.h"

// 日志函数声明
extern void logDebug(const std::string& message);

// 定义缺失的常量
#define D3D12_SIMULTANEOUS_RENDERTARGET_COUNT 8

// 前向声明
class RALRootParameter;
bool ConvertToD3D12RootParameter(const RALRootParameter& ralParam, D3D12_ROOT_PARAMETER& outParam);
class RALStaticSampler;
bool ConvertToD3D12StaticSampler(const RALStaticSampler& ralSampler, D3D12_STATIC_SAMPLER_DESC& outSampler);

// 定义一些常量
const uint32_t kDefaultFrameCount = 2;

// 构造函数
DX12RALDevice::DX12RALDevice(uint32_t width, uint32_t height, const std::wstring& windowName, HWND hWnd)
    : m_width(width), m_height(height), m_windowName(windowName), m_backBufferCount(kDefaultFrameCount), m_hWnd(hWnd),
      m_fenceValue(0), m_fenceEvent(nullptr), m_currentFrameIndex(0)
{
    // 相机现在在Main.cpp中初始化
}

// 析构函数
DX12RALDevice::~DX12RALDevice()
{
    Cleanup();
}

// 初始化DirectX 12
bool DX12RALDevice::Initialize()
{
    // 创建设备和交换链
    if (!CreateDeviceAndSwapChain())
    {
        return false;
    }

    // 创建命令对象
    CreateCommandObjects();

    // 创建描述符堆
    CreateDescriptorHeaps();

    // 创建渲染目标视图
    CreateMainRenderTargetViews();

    // 创建深度/模板视图
    CreateMainDepthStencilView();

    return true;
}

void DX12RALDevice::BeginFrame()
{
    ID3D12GraphicsCommandList* dx12CommandList = (ID3D12GraphicsCommandList*)m_graphicsCommandList->GetNativeCommandList();;

    // 获取当前后台缓冲区
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_mainRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_currentBackBufferIndex * m_rtvDescriptorSize;

    // 重置当前帧的命令分配器
    HRESULT hr = m_commandAllocators[m_currentFrameIndex]->Reset();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command allocator.");
    }

    // 重置当前帧的命令分配器
    hr = dx12CommandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), nullptr);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command list.");
    }

    // 资源转换：设置渲染目标为渲染状态
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_backBuffers[m_currentBackBufferIndex].Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    dx12CommandList->ResourceBarrier(1, &barrier);

    // 清除渲染目标和深度缓冲区 - 浅灰色背景
    const float clearColor[] = { 0.9f, 0.9f, 0.9f, 1.0f };

    dx12CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    dx12CommandList->ClearDepthStencilView(
			m_mainDsvHeap->GetCPUDescriptorHandleForHeapStart(),
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr
    );

    // 设置渲染目标和深度/模板视图
    dx12CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &m_mainDsvHeap->GetCPUDescriptorHandleForHeapStart());

    // 设置视口和裁剪矩形
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    dx12CommandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(m_width);
    scissorRect.bottom = static_cast<LONG>(m_height);
    dx12CommandList->RSSetScissorRects(1, &scissorRect);
}

//void DX12Renderer::ExecuteCommandLists(uint32_t count, IRALCommandList** ppCommandList)
//{
//    if (count > 0)
//    {
//        std::vector<ID3D12CommandList*> commandLists;
//
//        for (uint32_t i = 0; i < count; ++i)
//        {
//            IRALCommandList* pCommandList = ppCommandList[i];
//            commandLists.push_back((ID3D12CommandList*)pCommandList->GetNativeCommandList());
//        }
//
//        m_commandQueue->ExecuteCommandLists((UINT)count, commandLists.data());
//    }
//}

void DX12RALDevice::EndFrame()
{
    // 关闭命令列表
    ID3D12GraphicsCommandList* commandList = (ID3D12GraphicsCommandList*)m_graphicsCommandList->GetNativeCommandList();

    // 资源转换：设置渲染目标为呈现状态
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_backBuffers[m_currentBackBufferIndex].Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();

    ID3D12CommandList* ppCommandLists[] = { commandList };

    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 呈现
    HRESULT presentHr = m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    if (FAILED(presentHr))
    {
        throw std::runtime_error("Failed to present swap chain.");
    }

    // 等待当前帧完成
    WaitForPreviousFrame();

    m_uploadingResources.clear();
}

// 创建设备和交换链
bool DX12RALDevice::CreateDeviceAndSwapChain()
{
    // 在调试版本中启用D3D12调试层
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf()))))
    {
        debugController->EnableDebugLayer();
        // debugController会在离开作用域时自动释放
    }
#endif

    // 创建DXGI工厂
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create DXGI factory." << std::endl;
        return false;
    }

    // 查找支持DirectX 12的GPU
    bool foundAdapter = false;
    TRefCountPtr<IDXGIAdapter1> adapter;
    IDXGIAdapter1* adapterPtr = nullptr;
    for (uint32_t adapterIndex = 0; !foundAdapter && DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapterPtr); ++adapterIndex)
    {
        adapter = TRefCountPtr<IDXGIAdapter1>(adapterPtr);
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // 跳过软件适配器
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            // 尝试创建设备
            hr = D3D12CreateDevice(
                adapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf())
            );

            if (SUCCEEDED(hr))
            {
                foundAdapter = true;
            }
        }
    }

    if (!foundAdapter)
    {
        std::cerr << "Failed to find a suitable DirectX 12 adapter." << std::endl;
        return false;
    }

    // 创建命令队列描述
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    // 创建命令队列
    hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create command queue." << std::endl;
        return false;
    }

    // 创建交换链描述
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_backBufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    // 创建交换链
    ComPtr<IDXGISwapChain1> swapChain1;
    hr = m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        m_hWnd,  // 使用传入的窗口句柄
        &swapChainDesc,
        nullptr,
        nullptr,
        swapChain1.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        std::cerr << "Failed to create swap chain." << std::endl;
        return false;
    }

    // 提升到IDXGISwapChain4
    IDXGISwapChain4* swapChain4 = nullptr;
    hr = swapChain1.Get()->QueryInterface(IID_PPV_ARGS(&swapChain4));
    if (SUCCEEDED(hr))
    {
        m_swapChain = swapChain4;
    }
    
    if (FAILED(hr))
    {
        std::cerr << "Failed to upgrade swap chain to IDXGISwapChain4." << std::endl;
        return false;
    }

    // 获取当前后台缓冲区索引
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 创建围栏用于同步
    hr = m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create fence." << std::endl;
        return false;
    }

    // 创建围栏事件
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        std::cerr << "Failed to create fence event." << std::endl;
        return false;
    }

    m_RTVDescriptorHeaps.SetDevice(m_device);
    m_DSVDescriptorHeaps.SetDevice(m_device);
    m_SRVDescriptorHeaps.SetDevice(m_device);

    return true;
}

// 创建命令对象
void DX12RALDevice::CreateCommandObjects()
{
    // 创建两个命令分配器
    for (uint32_t i = 0; i < 2; ++i)
    {
        HRESULT hr = m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(m_commandAllocators[i].ReleaseAndGetAddressOf())
        );

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command allocator.");
        }
    }

    ComPtr<ID3D12GraphicsCommandList> commandList;

    // 创建命令列表
    HRESULT hr = m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[0].Get(), // 初始使用第一个命令分配器
        nullptr,
        IID_PPV_ARGS(commandList.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create command list.");
    }

    // 关闭命令列表（初始状态是打开的）
    commandList->Close();

    // 创建DX12RALGraphicsCommandList实例
    m_graphicsCommandList = new DX12RALGraphicsCommandList(m_commandAllocators[0].Get(), commandList.Get());
}

// 创建描述符堆
void DX12RALDevice::CreateDescriptorHeaps()
{
    // 获取描述符大小
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_RTVDescriptorHeaps.SetDescriptorSize(m_rtvDescriptorSize);
    m_DSVDescriptorHeaps.SetDescriptorSize(m_dsvDescriptorSize);
    m_SRVDescriptorHeaps.SetDescriptorSize(m_srvDescriptorSize);

    // 创建主渲染目标视图堆（用于后缓冲区）
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = m_backBufferCount;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_mainRtvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create main RTV heap.");
    }

    // 创建主深度/模板视图堆（用于主深度缓冲区）
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_mainDsvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create main DSV heap.");
    }

    // 创建主着色器资源视图堆
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 10;  // 预留一些描述符
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_mainSrvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create main SRV heap.");
    }
}

// 创建渲染目标视图
void DX12RALDevice::CreateMainRenderTargetViews()
{
    // 获取渲染目标视图堆的起始句柄
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_mainRtvHeap->GetCPUDescriptorHandleForHeapStart();

    // 为每个后台缓冲区创建渲染目标视图
    m_backBuffers.resize(m_backBufferCount);
    for (uint32_t i = 0; i < m_backBufferCount; ++i)
    {
        // 获取后台缓冲区
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backBuffers[i].ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to get swap chain buffer.");
        }

        // 创建渲染目标视图
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);

        // 移动到下一个描述符
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
}

// 创建深度/模板视图
void DX12RALDevice::CreateMainDepthStencilView()
{
    // 创建深度/模板缓冲区描述
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 创建深度/模板缓冲区堆属性
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_CLEAR_VALUE depthStencilClearValue{};
    depthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;  // 必须与资源格式匹配
    depthStencilClearValue.DepthStencil.Depth = 1.0f;    // 深度缓冲区默认清除值
    depthStencilClearValue.DepthStencil.Stencil = 0;     // 模板缓冲区默认清除值

    // 创建深度/模板缓冲区
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthStencilClearValue,
        IID_PPV_ARGS(m_depthStencilBuffer.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create depth stencil buffer.");
    }

    // 创建深度/模板视图
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    m_device->CreateDepthStencilView(
        m_depthStencilBuffer.Get(),
        &dsvDesc,
        m_mainDsvHeap->GetCPUDescriptorHandleForHeapStart()
    );
}

// 通用着色器编译辅助方法
ComPtr<ID3DBlob> DX12RALDevice::CompileShaderBlob(const char* shaderCode, const char* entryPoint, const char* target)
{
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(
        shaderCode,
        strlen(shaderCode),
        nullptr,  // 可选的文件名
        nullptr,  // 可选的宏定义
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        0,        // 编译选项
        0,        // 效果标志
        shaderBlob.ReleaseAndGetAddressOf(),
        errorBlob.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string errorMsg = "Failed to compile shader: " + std::string(static_cast<char*>(errorBlob->GetBufferPointer()));
            logDebug("[DEBUG] " + errorMsg);
            std::cerr << errorMsg << std::endl;
        }
        else
        {
            std::string errorMsg = "Failed to compile shader without detailed error. HRESULT: " + std::to_string(hr);
            logDebug("[DEBUG] " + errorMsg);
            std::cerr << errorMsg << std::endl;
        }
        return nullptr;
    }

    logDebug("[DEBUG] Shader compiled successfully: " + std::string(entryPoint) + " (" + std::string(target) + ")");
    return shaderBlob;
}

// 编译顶点着色器
IRALVertexShader* DX12RALDevice::CompileVertexShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "vs_5_0");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto vertexShader = new DX12RALVertexShader();
    vertexShader->SetNativeShader(shaderBlob.Get());
    return vertexShader;
}

// 编译像素着色器
IRALPixelShader* DX12RALDevice::CompilePixelShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "ps_5_0");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto pixelShader = new DX12RALPixelShader();
    pixelShader->SetNativeShader(shaderBlob.Get());
    return pixelShader;
}

// 编译几何着色器
IRALGeometryShader* DX12RALDevice::CompileGeometryShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "gs_5_0");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto geometryShader = new DX12RALGeometryShader();
    geometryShader->SetNativeShader(shaderBlob.Get());
    return geometryShader;
}

// 编译计算着色器
IRALComputeShader* DX12RALDevice::CompileComputeShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "cs_5_0");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto computeShader = new DX12RALComputeShader();
    computeShader->SetNativeShader(shaderBlob.Get());
    return computeShader;
}

// 编译网格着色器
IRALMeshShader* DX12RALDevice::CompileMeshShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "ms_6_5");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto meshShader = new DX12RALMeshShader();
    meshShader->SetNativeShader(shaderBlob.Get());
    return meshShader;
}

// 编译放大着色器
IRALAmplificationShader* DX12RALDevice::CompileAmplificationShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "as_6_5");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto amplificationShader = new DX12RALAmplificationShader();
    amplificationShader->SetNativeShader(shaderBlob.Get());
    return amplificationShader;
}

// 编译光线生成着色器
IRALRayGenShader* DX12RALDevice::CompileRayGenShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "lib_6_3");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto rayGenShader = new DX12RALRayGenShader();
    rayGenShader->SetNativeShader(shaderBlob.Get());
    return rayGenShader;
}

// 编译光线未命中着色器
IRALRayMissShader* DX12RALDevice::CompileRayMissShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "lib_6_3");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto rayMissShader = new DX12RALRayMissShader();
    rayMissShader->SetNativeShader(shaderBlob.Get());
    return rayMissShader;
}

// 编译光线命中组着色器
IRALRayHitGroupShader* DX12RALDevice::CompileRayHitGroupShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "lib_6_3");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto rayHitGroupShader = new DX12RALRayHitGroupShader();
    rayHitGroupShader->SetNativeShader(shaderBlob.Get());
    return rayHitGroupShader;
}

// 编译光线可调用着色器
IRALRayCallableShader* DX12RALDevice::CompileRayCallableShader(const char* shaderCode, const char* entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob = CompileShaderBlob(shaderCode, entryPoint, "lib_6_3");
    if (!shaderBlob)
    {
        return nullptr;
    }

    auto rayCallableShader = new DX12RALRayCallableShader();
    rayCallableShader->SetNativeShader(shaderBlob.Get());
    return rayCallableShader;
}

// 创建图形管线状态
IRALGraphicsPipelineState* DX12RALDevice::CreateGraphicsPipelineState(const RALGraphicsPipelineStateDesc& desc)
{
    // 创建D3D12图形管线状态描述
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // 假设inputLayout是std::vector<RALVertexAttribute>*
    std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputLayout;

    // 设置输入布局
    if (desc.inputLayout) {
        for (const auto& attr : *desc.inputLayout) {
            D3D12_INPUT_ELEMENT_DESC element = {};
            // 设置元素名称
            switch (attr.semantic) {
            case RALVertexSemantic::Position:
                element.SemanticName = "POSITION";
                break;
            case RALVertexSemantic::Normal:
                element.SemanticName = "NORMAL";
                break;
            case RALVertexSemantic::Tangent:
                element.SemanticName = "TANGENT";
                break;
            case RALVertexSemantic::Bitangent:
                element.SemanticName = "BINORMAL";
                break;
            case RALVertexSemantic::TexCoord0:
                element.SemanticName = "TEXCOORD";
                element.SemanticIndex = 0;
                break;
            case RALVertexSemantic::TexCoord1:
                element.SemanticName = "TEXCOORD";
                element.SemanticIndex = 1;
                break;
            case RALVertexSemantic::Color0:
                element.SemanticName = "COLOR";
                element.SemanticIndex = 0;
                break;
            case RALVertexSemantic::Color1:
                element.SemanticName = "COLOR";
                element.SemanticIndex = 1;
                break;
            case RALVertexSemantic::BoneIndices:
                element.SemanticName = "BLENDINDICES";
                break;
            case RALVertexSemantic::BoneWeights:
                element.SemanticName = "BLENDWEIGHT";
                break;
            default:
                element.SemanticName = "UNKNOWN";
                break;
            }

            // 设置格式
            switch (attr.format) {
            case RALVertexFormat::Float1:
                element.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case RALVertexFormat::Float2:
                element.Format = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case RALVertexFormat::Float3:
                element.Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;
            case RALVertexFormat::Float4:
                element.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            case RALVertexFormat::UByte4N:
                element.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case RALVertexFormat::Byte4N:
                element.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
                break;
            default:
                element.Format = DXGI_FORMAT_UNKNOWN;
                break;
            }

            element.InputSlot = attr.bufferSlot;
            element.AlignedByteOffset = attr.offset;
            element.InputSlotClass = attr.bufferSlot >= 1 ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            element.InstanceDataStepRate = attr.bufferSlot >= 1 ? 1 : 0;

            d3dInputLayout.push_back(element);
        }

        psoDesc.InputLayout = { d3dInputLayout.data(), static_cast<UINT>(d3dInputLayout.size()) };
    }

    // 设置根签名
    if (desc.rootSignature) {
        psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(desc.rootSignature->GetNativeResource());
    }

    // 设置顶点着色器
    if (desc.vertexShader) {
        auto dx12VertexShader = static_cast<DX12RALVertexShader*>(desc.vertexShader);
        ID3DBlob* shaderBlob = dx12VertexShader->GetNativeShader();
        if (shaderBlob) {
            psoDesc.VS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };
        }
    }

    // 设置像素着色器
    if (desc.pixelShader) {
        auto dx12PixelShader = static_cast<DX12RALPixelShader*>(desc.pixelShader);
        ID3DBlob* shaderBlob = dx12PixelShader->GetNativeShader();
        if (shaderBlob) {
            psoDesc.PS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };
        }
    }

    // 设置几何着色器
    if (desc.geometryShader) {
        auto dx12GeometryShader = static_cast<DX12RALGeometryShader*>(desc.geometryShader);
        ID3DBlob* shaderBlob = dx12GeometryShader->GetNativeShader();
        if (shaderBlob) {
            psoDesc.GS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };
        }
    }

    // 设置图元拓扑类型
    switch (desc.primitiveTopologyType) {
    case RALPrimitiveTopologyType::TriangleList:
    case RALPrimitiveTopologyType::TriangleStrip:
    case RALPrimitiveTopologyType::TriangleListAdj:
    case RALPrimitiveTopologyType::TriangleStripAdj:
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case RALPrimitiveTopologyType::LineList:
    case RALPrimitiveTopologyType::LineStrip:
    case RALPrimitiveTopologyType::LineListAdj:
    case RALPrimitiveTopologyType::LineStripAdj:
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case RALPrimitiveTopologyType::PointList:
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        break;
    default:
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    }

    // 设置光栅化状态
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    switch (desc.rasterizerState.fillMode) {
    case RALFillMode::Wireframe:
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    case RALFillMode::Solid:
    default:
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        break;
    }

    switch (desc.rasterizerState.cullMode) {
    case RALCullMode::None:
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        break;
    case RALCullMode::Front:
        rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
        break;
    case RALCullMode::Back:
    default:
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        break;
    }

    rasterizerDesc.FrontCounterClockwise = desc.rasterizerState.frontCounterClockwise;
    rasterizerDesc.DepthBias = desc.rasterizerState.depthBias;
    rasterizerDesc.DepthBiasClamp = desc.rasterizerState.depthBiasClamp;
    rasterizerDesc.SlopeScaledDepthBias = desc.rasterizerState.slopeScaledDepthBias;
    rasterizerDesc.DepthClipEnable = desc.rasterizerState.depthClipEnable;
    rasterizerDesc.MultisampleEnable = desc.rasterizerState.multisampleEnable;
    rasterizerDesc.AntialiasedLineEnable = desc.rasterizerState.antialiasedLineEnable;
    rasterizerDesc.ForcedSampleCount = desc.rasterizerState.forcedSampleCount;
    rasterizerDesc.ConservativeRaster = desc.rasterizerState.conservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.RasterizerState = rasterizerDesc;

    // 设置混合状态
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = desc.blendState.alphaToCoverageEnable;
    blendDesc.IndependentBlendEnable = desc.blendState.independentBlendEnable;

    for (uint32_t i = 0; i < D3D12_SIMULTANEOUS_RENDERTARGET_COUNT && i < desc.numRenderTargets; ++i) {
        const auto& rtBlendState = desc.renderTargetBlendStates[i];
        auto& rtBlendDesc = blendDesc.RenderTarget[i];

        rtBlendDesc.BlendEnable = rtBlendState.blendEnable;
        rtBlendDesc.LogicOpEnable = rtBlendState.logicOpEnable;

        // 设置源混合因子和目标混合因子
        switch (rtBlendState.srcBlend) {
        case RALBlendFactor::Zero:
            rtBlendDesc.SrcBlend = D3D12_BLEND_ZERO;
            break;
        case RALBlendFactor::One:
            rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
            break;
        case RALBlendFactor::SourceColor:
            rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_COLOR;
            break;
        case RALBlendFactor::OneMinusSourceColor:
            rtBlendDesc.SrcBlend = D3D12_BLEND_INV_SRC_COLOR;
            break;
        case RALBlendFactor::SourceAlpha:
            rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            break;
        case RALBlendFactor::OneMinusSourceAlpha:
            rtBlendDesc.SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case RALBlendFactor::DestinationAlpha:
            rtBlendDesc.SrcBlend = D3D12_BLEND_DEST_ALPHA;
            break;
        case RALBlendFactor::OneMinusDestinationAlpha:
            rtBlendDesc.SrcBlend = D3D12_BLEND_INV_DEST_ALPHA;
            break;
        case RALBlendFactor::DestinationColor:
            rtBlendDesc.SrcBlend = D3D12_BLEND_DEST_COLOR;
            break;
        case RALBlendFactor::OneMinusDestinationColor:
            rtBlendDesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
            break;
        case RALBlendFactor::SourceAlphaSaturate:
            rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA_SAT;
            break;
        default:
            // 使用默认值
            break;
        }

        // 目标混合因子
        switch (rtBlendState.destBlend) {
        case RALBlendFactor::Zero:
            rtBlendDesc.DestBlend = D3D12_BLEND_ZERO;
            break;
        case RALBlendFactor::One:
            rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
            break;
        case RALBlendFactor::SourceColor:
            rtBlendDesc.DestBlend = D3D12_BLEND_SRC_COLOR;
            break;
        case RALBlendFactor::OneMinusSourceColor:
            rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
            break;
        case RALBlendFactor::SourceAlpha:
            rtBlendDesc.DestBlend = D3D12_BLEND_SRC_ALPHA;
            break;
        case RALBlendFactor::OneMinusSourceAlpha:
            rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case RALBlendFactor::DestinationAlpha:
            rtBlendDesc.DestBlend = D3D12_BLEND_DEST_ALPHA;
            break;
        case RALBlendFactor::OneMinusDestinationAlpha:
            rtBlendDesc.DestBlend = D3D12_BLEND_INV_DEST_ALPHA;
            break;
        case RALBlendFactor::DestinationColor:
            rtBlendDesc.DestBlend = D3D12_BLEND_DEST_COLOR;
            break;
        case RALBlendFactor::OneMinusDestinationColor:
            rtBlendDesc.DestBlend = D3D12_BLEND_INV_DEST_COLOR;
            break;
        default:
            // 使用默认值
            break;
        }

        // 混合操作
        switch (rtBlendState.blendOp) {
        case RALBlendOp::Add:
            rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
            break;
        case RALBlendOp::Subtract:
            rtBlendDesc.BlendOp = D3D12_BLEND_OP_SUBTRACT;
            break;
        case RALBlendOp::ReverseSubtract:
            rtBlendDesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
            break;
        case RALBlendOp::Min:
            rtBlendDesc.BlendOp = D3D12_BLEND_OP_MIN;
            break;
        case RALBlendOp::Max:
            rtBlendDesc.BlendOp = D3D12_BLEND_OP_MAX;
            break;
        default:
            // 使用默认值
            break;
        }

        // Alpha混合因子和操作（类似RGB部分）
        switch (rtBlendState.srcBlendAlpha) {
        case RALBlendFactor::Zero:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ZERO;
            break;
        case RALBlendFactor::One:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
            break;
        case RALBlendFactor::SourceColor:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_COLOR;
            break;
        case RALBlendFactor::OneMinusSourceColor:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_INV_SRC_COLOR;
            break;
        case RALBlendFactor::SourceAlpha:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
            break;
        case RALBlendFactor::OneMinusSourceAlpha:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case RALBlendFactor::DestinationAlpha:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
            break;
        case RALBlendFactor::OneMinusDestinationAlpha:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
            break;
        case RALBlendFactor::DestinationColor:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_DEST_COLOR;
            break;
        case RALBlendFactor::OneMinusDestinationColor:
            rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_INV_DEST_COLOR;
            break;
        default:
            // 使用默认值
            break;
        }

        switch (rtBlendState.destBlendAlpha) {
        case RALBlendFactor::Zero:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
            break;
        case RALBlendFactor::One:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
            break;
        case RALBlendFactor::SourceColor:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_SRC_COLOR;
            break;
        case RALBlendFactor::OneMinusSourceColor:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_COLOR;
            break;
        case RALBlendFactor::SourceAlpha:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
            break;
        case RALBlendFactor::OneMinusSourceAlpha:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;
        case RALBlendFactor::DestinationAlpha:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
            break;
        case RALBlendFactor::OneMinusDestinationAlpha:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
            break;
        case RALBlendFactor::DestinationColor:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_DEST_COLOR;
            break;
        case RALBlendFactor::OneMinusDestinationColor:
            rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_DEST_COLOR;
            break;
        default:
            // 使用默认值
            break;
        }

        switch (rtBlendState.blendOpAlpha) {
        case RALBlendOp::Add:
            rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            break;
        case RALBlendOp::Subtract:
            rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_SUBTRACT;
            break;
        case RALBlendOp::ReverseSubtract:
            rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_REV_SUBTRACT;
            break;
        case RALBlendOp::Min:
            rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_MIN;
            break;
        case RALBlendOp::Max:
            rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_MAX;
            break;
        default:
            // 使用默认值
            break;
        }

        // 逻辑操作
        switch (rtBlendState.logicOp) {
        case RALLogicOp::Copy:
            rtBlendDesc.LogicOp = D3D12_LOGIC_OP_COPY;
            break;
        case RALLogicOp::Noop:
            rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
            break;
        case RALLogicOp::Set:
            rtBlendDesc.LogicOp = D3D12_LOGIC_OP_SET;
            break;
            // RALLogicOp枚举中没有Clear和Invert，这里不做处理
            // case RALLogicOp::Clear:
            //    rtBlendDesc.LogicOp = D3D12_LOGIC_OP_CLEAR;
            //    break;
            // case RALLogicOp::Invert:
            //    rtBlendDesc.LogicOp = D3D12_LOGIC_OP_INVERT;
            //    break;
        case RALLogicOp::Or:
            // D3D12_LOGIC_OP_OR在某些版本可能不可用，使用等效操作
            break;
        case RALLogicOp::And:
            // D3D12_LOGIC_OP_AND在某些版本可能不可用，使用等效操作
            break;
        case RALLogicOp::Xor:
            // D3D12_LOGIC_OP_XOR在某些版本可能不可用，使用等效操作
            break;
        case RALLogicOp::Nor:
            // D3D12_LOGIC_OP_NOR在某些版本可能不可用，使用等效操作
            break;
        case RALLogicOp::Nand:
            // D3D12_LOGIC_OP_NAND在某些版本可能不可用，使用等效操作
            break;
        case RALLogicOp::Equiv:
            // D3D12_LOGIC_OP_EQUIV在某些版本可能不可用，使用等效操作
            break;
        default:
            // 默认使用NOOP
            break;
        }

        // 颜色写入掩码
        rtBlendDesc.RenderTargetWriteMask = static_cast<uint8_t>(rtBlendState.colorWriteMask);
    }

    psoDesc.BlendState = blendDesc;

    // 设置深度模板状态
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = desc.depthStencilState.depthEnable;
    depthStencilDesc.DepthWriteMask = desc.depthStencilState.depthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

    switch (desc.depthStencilState.depthFunc) {
    case RALCompareOp::Never:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case RALCompareOp::Less:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    case RALCompareOp::Equal:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        break;
    case RALCompareOp::LessOrEqual:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        break;
    case RALCompareOp::Greater:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        break;
    case RALCompareOp::NotEqual:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
        break;
    case RALCompareOp::GreaterOrEqual:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        break;
    case RALCompareOp::Always:
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    default:
        // 默认使用LESS
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    }

    depthStencilDesc.StencilEnable = desc.depthStencilState.stencilEnable;
    depthStencilDesc.StencilReadMask = desc.depthStencilState.stencilReadMask;
    depthStencilDesc.StencilWriteMask = desc.depthStencilState.stencilWriteMask;

    // 设置前向面模板操作
    switch (desc.depthStencilState.frontFace.failOp) {
    case RALStencilOp::Keep:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        break;
    case RALStencilOp::Zero:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_ZERO;
        break;
    case RALStencilOp::Replace:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
        break;
    case RALStencilOp::IncrementClamp:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INCR_SAT;
        break;
    case RALStencilOp::DecrementClamp:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_DECR_SAT;
        break;
    case RALStencilOp::Invert:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INVERT;
        break;
    case RALStencilOp::IncrementWrap:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INCR;
        break;
    case RALStencilOp::DecrementWrap:
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_DECR;
        break;
    default:
        // 默认使用KEEP
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        break;
    }

    switch (desc.depthStencilState.frontFace.depthFailOp) {
    case RALStencilOp::Keep:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        break;
    case RALStencilOp::Zero:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_ZERO;
        break;
    case RALStencilOp::Replace:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
        break;
    case RALStencilOp::IncrementClamp:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR_SAT;
        break;
    case RALStencilOp::DecrementClamp:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR_SAT;
        break;
    case RALStencilOp::Invert:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INVERT;
        break;
    case RALStencilOp::IncrementWrap:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
        break;
    case RALStencilOp::DecrementWrap:
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
        break;
    default:
        // 默认使用KEEP
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        break;
    }

    switch (desc.depthStencilState.frontFace.passOp) {
    case RALStencilOp::Keep:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        break;
    case RALStencilOp::Zero:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
        break;
    case RALStencilOp::Replace:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
        break;
    case RALStencilOp::IncrementClamp:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
        break;
    case RALStencilOp::DecrementClamp:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_DECR_SAT;
        break;
    case RALStencilOp::Invert:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INVERT;
        break;
    case RALStencilOp::IncrementWrap:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
        break;
    case RALStencilOp::DecrementWrap:
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_DECR;
        break;
    default:
        // 默认使用KEEP
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        break;
    }

    switch (desc.depthStencilState.frontFace.compareFunc) {
    case RALCompareOp::Never:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case RALCompareOp::Less:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    case RALCompareOp::Equal:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
        break;
    case RALCompareOp::LessOrEqual:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        break;
    case RALCompareOp::Greater:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;
        break;
    case RALCompareOp::NotEqual:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
        break;
    case RALCompareOp::GreaterOrEqual:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        break;
    case RALCompareOp::Always:
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    default:
        // 默认使用ALWAYS
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    }

    // 设置后向面模板操作（与前向面相同）
    depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
    psoDesc.DepthStencilState = depthStencilDesc;

    // 设置采样器掩码和渲染目标格式
    psoDesc.SampleMask = desc.sampleMask;
    psoDesc.NumRenderTargets = desc.numRenderTargets;

    for (uint32_t i = 0; i < desc.numRenderTargets && i < D3D12_SIMULTANEOUS_RENDERTARGET_COUNT; ++i) {
        psoDesc.RTVFormats[i] = toDXGIFormat(desc.renderTargetFormats[i]);
    }

    psoDesc.DSVFormat = toDXGIFormat(desc.depthStencilFormat);

    // 设置采样描述
    psoDesc.SampleDesc.Count = desc.sampleDesc.Count;
    psoDesc.SampleDesc.Quality = desc.sampleDesc.Quality;

    // 创建D3D12管线状态对象
    ComPtr<ID3D12PipelineState> pipelineState;
    HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
        return nullptr;
    }

    // 创建并返回RAL图形管线状态对象
    auto ralPipelineState = new DX12RALGraphicsPipelineState();
    ralPipelineState->SetNativePipelineState(pipelineState.Get());

    return ralPipelineState;
}

// 根签名标志位运算符重载
template<typename EnumType>
constexpr typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type
operator|(EnumType lhs, EnumType rhs)
{
    using UnderlyingType = typename std::underlying_type<EnumType>::type;
    return static_cast<EnumType>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
}

template<typename EnumType>
constexpr typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type
operator&(EnumType lhs, EnumType rhs)
{
    using UnderlyingType = typename std::underlying_type<EnumType>::type;
    return static_cast<EnumType>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
}

template<typename EnumType>
constexpr typename std::enable_if<std::is_enum<EnumType>::value, bool>::type
operator!(EnumType value)
{
    using UnderlyingType = typename std::underlying_type<EnumType>::type;
    return static_cast<UnderlyingType>(value) == 0;
}

// 将RALRootSignatureFlags转换为D3D12_ROOT_SIGNATURE_FLAGS
static inline D3D12_ROOT_SIGNATURE_FLAGS ConvertToD3D12RootSignatureFlags(RALRootSignatureFlags flags)
{
    D3D12_ROOT_SIGNATURE_FLAGS d3d12Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(RALRootSignatureFlags::AllowInputAssemblerInputLayout))
        d3d12Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(RALRootSignatureFlags::AllowStreamOutput))
        d3d12Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
    // 仅保留已定义的D3D12标志
    
    return d3d12Flags;
}

// 创建根签名
IRALRootSignature* DX12RALDevice::CreateRootSignature(const std::vector<RALRootParameter>& rootParameters,
    const std::vector<RALStaticSampler>& staticSamplers, RALRootSignatureFlags flags)
{
    // 转换根参数
    std::vector<D3D12_ROOT_PARAMETER> d3d12RootParameters;
    d3d12RootParameters.reserve(rootParameters.size());
    
    for (const auto& ralParam : rootParameters)
    {
        D3D12_ROOT_PARAMETER d3d12Param;
        if (!ConvertToD3D12RootParameter(ralParam, d3d12Param)) {
            std::cerr << "Failed to convert RALRootParameter to D3D12_ROOT_PARAMETER" << std::endl;
            logDebug("[DEBUG] Failed to convert RALRootParameter to D3D12_ROOT_PARAMETER");
            return nullptr;
        }
        
        d3d12RootParameters.push_back(d3d12Param);
    }
    
    // 转换静态采样器
    std::vector<D3D12_STATIC_SAMPLER_DESC> d3d12StaticSamplers(staticSamplers.size());
    for (size_t i = 0; i < staticSamplers.size(); ++i)
    {
        if (!ConvertToD3D12StaticSampler(staticSamplers[i], d3d12StaticSamplers[i]))
        {
            std::cerr << "Failed to convert RALStaticSampler to D3D12_STATIC_SAMPLER_DESC at index " << i << std::endl;
            logDebug("[DEBUG] Failed to convert RALStaticSampler to D3D12_STATIC_SAMPLER_DESC at index (" + std::to_string(i) + ")");
            return nullptr;
        }
    }
    
    // 创建根签名描述
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = static_cast<uint32_t>(d3d12RootParameters.size());
    rootSignatureDesc.pParameters = d3d12RootParameters.data();
    rootSignatureDesc.NumStaticSamplers = static_cast<uint32_t>(d3d12StaticSamplers.size());
    rootSignatureDesc.pStaticSamplers = d3d12StaticSamplers.data();
    rootSignatureDesc.Flags = ConvertToD3D12RootSignatureFlags(flags);
    
    // 序列化根签名
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,
        rootSignatureBlob.ReleaseAndGetAddressOf(),
        errorBlob.ReleaseAndGetAddressOf()
    );
    
    // 清理堆上分配的描述符表范围内存
    for (auto& param : d3d12RootParameters)
    {
        if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            delete[] param.DescriptorTable.pDescriptorRanges;
        }
    }
    
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string errorMsg = "Failed to serialize root signature: " + std::string(static_cast<char*>(errorBlob->GetBufferPointer()));
            logDebug("[DEBUG] " + errorMsg);
            std::cerr << errorMsg << std::endl;
        }
        logDebug("[DEBUG] Failed to serialize root signature");
        return nullptr;
    }
    
    // 创建根签名
    ComPtr<ID3D12RootSignature> d3d12RootSignature;
    hr = m_device->CreateRootSignature(
        0,
        rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(d3d12RootSignature.ReleaseAndGetAddressOf())
    );
    
    if (FAILED(hr))
    {
        std::cerr << "Failed to create root signature. HRESULT: " << hr << std::endl;
        logDebug("[DEBUG] Failed to create root signature");
        return nullptr;
    }
    
    logDebug("[DEBUG] Root signature created successfully with RALRootParameter");
    
    // 创建并返回IRALRootSignature对象
    auto ralRootSignature = new DX12RALRootSignature();
    static_cast<DX12RALRootSignature*>(ralRootSignature)->SetNativeRootSignature(d3d12RootSignature.Get());
    
    return ralRootSignature;
}

// 前向声明
bool ConvertToD3D12RootParameter(const RALRootParameter& ralParam, D3D12_ROOT_PARAMETER& outParam);

// 将RALStaticSampler转换为D3D12_STATIC_SAMPLER_DESC
bool ConvertToD3D12StaticSampler(const RALStaticSampler& ralSampler, D3D12_STATIC_SAMPLER_DESC& outSampler)
{
    outSampler = {};
    
    // 转换过滤模式
    switch (ralSampler.Filter)
    {
    case RALFilter::MinMagMipLinear:
        outSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case RALFilter::Anisotropic:
        outSampler.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
    case RALFilter::ComparisonMinMagMipLinear:
        outSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        break;
    case RALFilter::ComparisonAnisotropic:
        outSampler.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
        break;
    // 这里可以添加更多过滤模式的映射
    default:
        outSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    }
    
    // 转换纹理寻址模式
    switch (ralSampler.AddressU)
    {
    case RALTextureAddressMode::Wrap:
        outSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case RALTextureAddressMode::Mirror:
        outSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        break;
    case RALTextureAddressMode::Clamp:
        outSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    case RALTextureAddressMode::Border:
        outSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        break;
    case RALTextureAddressMode::MirrorOnce:
        outSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        break;
    default:
        outSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    }
    
    // 复制AddressU的设置到AddressV和AddressW
    outSampler.AddressV = outSampler.AddressU;
    outSampler.AddressW = outSampler.AddressU;
    
    // 设置其他参数
    outSampler.MipLODBias = ralSampler.MipLODBias;
    outSampler.MaxAnisotropy = ralSampler.MaxAnisotropy;
    
    // 转换比较函数
    switch (ralSampler.ComparisonFunc)
    {
    case RALComparisonFunc::Never:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case RALComparisonFunc::Less:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    case RALComparisonFunc::Equal:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;
        break;
    case RALComparisonFunc::LessEqual:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        break;
    case RALComparisonFunc::Greater:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
        break;
    case RALComparisonFunc::NotEqual:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
        break;
    case RALComparisonFunc::GreaterEqual:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        break;
    case RALComparisonFunc::Always:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    default:
        outSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    }
    
    // 转换边界颜色
    switch (ralSampler.BorderColor)
    {
    case RALStaticBorderColor::TransparentBlack:
        outSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        break;
    case RALStaticBorderColor::OpaqueBlack:
        outSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        break;
    case RALStaticBorderColor::OpaqueWhite:
        outSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        break;
    default:
        outSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        break;
    }
    
    // 设置LOD范围
    outSampler.MinLOD = ralSampler.MinLOD;
    outSampler.MaxLOD = ralSampler.MaxLOD;
    
    // 设置着色器寄存器和空间
    outSampler.ShaderRegister = ralSampler.ShaderRegister;
    outSampler.RegisterSpace = ralSampler.RegisterSpace;
    
    // 转换着色器可见性
    outSampler.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(ralSampler.ShaderVisibility);
    
    return true;
}

// 将RALRootParameter转换为D3D12_ROOT_PARAMETER
bool ConvertToD3D12RootParameter(const RALRootParameter& ralParam, D3D12_ROOT_PARAMETER& outParam)
{
    outParam = {};
    
    switch (ralParam.Type)
    {
    case RALRootParameterType::Constant:
        {
            outParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            outParam.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(ralParam.ShaderVisibility);
            outParam.Constants.ShaderRegister = ralParam.Data.Constants[0];
            outParam.Constants.RegisterSpace = ralParam.Data.Constants[1];
            outParam.Constants.Num32BitValues = ralParam.Data.Constants[2];
        }
        break;
        
    case RALRootParameterType::ConstantBufferView:
        {
            outParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            outParam.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(ralParam.ShaderVisibility);
            outParam.Descriptor.ShaderRegister = ralParam.Data.Descriptor.ShaderRegister;
            outParam.Descriptor.RegisterSpace = ralParam.Data.Descriptor.RegisterSpace;
        }
        break;
        
    case RALRootParameterType::ShaderResourceView:
        {
            outParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            outParam.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(ralParam.ShaderVisibility);
            outParam.Descriptor.ShaderRegister = ralParam.Data.Descriptor.ShaderRegister;
            outParam.Descriptor.RegisterSpace = ralParam.Data.Descriptor.RegisterSpace;
        }
        break;
        
    case RALRootParameterType::UnorderedAccessView:
        {
            outParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
            outParam.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(ralParam.ShaderVisibility);
            outParam.Descriptor.ShaderRegister = ralParam.Data.Descriptor.ShaderRegister;
            outParam.Descriptor.RegisterSpace = ralParam.Data.Descriptor.RegisterSpace;
        }
        break;
        
    case RALRootParameterType::DescriptorTable:
        {
            outParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            outParam.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(ralParam.ShaderVisibility);
            
            // 转换描述符表范围
            std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
            ranges.reserve(ralParam.Data.DescriptorTable.Ranges.size());
            
            for (const auto& ralRange : ralParam.Data.DescriptorTable.Ranges)
            {
                D3D12_DESCRIPTOR_RANGE range = {};
                
                // 根据RALDescriptorRangeType设置D3D12_DESCRIPTOR_RANGE_TYPE
                switch (ralRange.Type)
                {
                case RALDescriptorRangeType::SRV:
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    break;
                case RALDescriptorRangeType::UAV:
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    break;
                case RALDescriptorRangeType::CBV:
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    break;
                case RALDescriptorRangeType::Sampler:
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    break;
                default:
                    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 默认为CBV
                    break;
                }
                
                range.NumDescriptors = ralRange.NumDescriptors;
                range.BaseShaderRegister = ralRange.BaseShaderRegister;
                range.RegisterSpace = ralRange.RegisterSpace;
                ranges.push_back(range);
            }
            
            // 需要在堆上分配内存以避免生命周期问题
            D3D12_DESCRIPTOR_RANGE* pRanges = new D3D12_DESCRIPTOR_RANGE[ranges.size()];
            std::copy(ranges.begin(), ranges.end(), pRanges);
            
            outParam.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(ranges.size());
            outParam.DescriptorTable.pDescriptorRanges = pRanges;
        }
        break;
        
    default:
        logDebug("[DEBUG] Invalid RALRootParameterType");
        return false;
    }
    
    return true;
}

// 创建缓冲区
ComPtr<ID3D12Resource> DX12RALDevice::CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
    D3D12_HEAP_PROPERTIES heapProps,
    D3D12_RESOURCE_STATES initialState)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;

    ComPtr<ID3D12Resource> buffer;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create buffer.");
    }

    return buffer;
}

// 等待操作完成
void DX12RALDevice::WaitForPreviousOperations()
{
    // 推进围栏值
    uint64_t currentFenceValue = ++m_fenceValue;

    // 向命令队列添加围栏
    HRESULT hr = m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to signal fence.");
    }

    // 如果围栏值尚未完成，则等待
    if (m_fence->GetCompletedValue() < currentFenceValue)
    {
        hr = m_fence->SetEventOnCompletion(currentFenceValue, m_fenceEvent);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to set event on fence completion.");
        }

        // 等待事件
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

// 等待前一帧完成
void DX12RALDevice::WaitForPreviousFrame()
{
    WaitForPreviousOperations();

    // 切换到下一帧的命令分配器
    m_currentFrameIndex = (m_currentFrameIndex + 1) % 2;
}

// 调整窗口大小
void DX12RALDevice::Resize(uint32_t width, uint32_t height)
{
    // 等待所有命令完成
    WaitForPreviousFrame();

    // 保存新的窗口尺寸
    m_width = width;
    m_height = height;

    // 释放旧的渲染目标视图和深度/模板视图
    m_backBuffers.clear();
    m_depthStencilBuffer.Reset();

    // 调整交换链大小
    HRESULT hr = m_swapChain->ResizeBuffers(
        m_backBufferCount,
        m_width,
        m_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to resize swap chain buffers.");
    }

    // 重新创建渲染目标视图
    CreateMainRenderTargetViews();

    // 重新创建深度/模板视图
    CreateMainDepthStencilView();
}

//// 创建图形命令列表
//IRALGraphicsCommandList* DX12Renderer::CreateGraphicsCommandList()
//{
//    ComPtr<ID3D12GraphicsCommandList> commandList;
//    HRESULT hr = m_device->CreateCommandList(
//        0,                                              // 节点掩码
//        D3D12_COMMAND_LIST_TYPE_DIRECT,                 // 命令列表类型
//        m_commandAllocators[m_currentFrameIndex].Get(),  // 命令分配器
//        nullptr,                                        // 初始管道状态对象
//        IID_PPV_ARGS(commandList.ReleaseAndGetAddressOf()));
//
//    if (FAILED(hr))
//    {
//        return nullptr;
//    }
//    else
//    {
//        // 使用当前帧的命令分配器创建图形命令列表
//        return new DX12RALGraphicsCommandList(m_commandAllocators[m_currentFrameIndex].Get(), commandList.Get());
//    }
//}

// 创建顶点缓冲区
IRALVertexBuffer* DX12RALDevice::CreateVertexBuffer(uint32_t size, uint32_t stride, bool isStatic)
{
    // 创建DX12RALVertexBuffer对象
    DX12RALVertexBuffer* vertexBuffer = new DX12RALVertexBuffer(size, stride);

    // 设置堆属性
    D3D12_HEAP_PROPERTIES heapProps = {};
    if (isStatic)
    {
        // 静态缓冲区使用DEFAULT堆，适合GPU读取
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
    }
    else 
    {
        // 动态缓冲区使用UPLOAD堆，适合CPU写入GPU读取
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
    }

    // 设置初始状态
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    // 创建底层D3D12资源
    ComPtr<ID3D12Resource> d3d12Resource = CreateBuffer(
        size,
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        initialState
    );

    // 设置原生资源
    vertexBuffer->SetNativeResource(d3d12Resource.Get());
    vertexBuffer->SetResourceState(RALResourceState::VertexBuffer);

    return vertexBuffer;
}

// 创建索引缓冲区
IRALIndexBuffer* DX12RALDevice::CreateIndexBuffer(uint32_t count, bool is32BitIndex, bool isStatic)
{
    // 创建DX12RALIndexBuffer对象
    uint32_t size = is32BitIndex ? count * sizeof(int32_t) : count * sizeof(int16_t);

    DX12RALIndexBuffer* indexBuffer = new DX12RALIndexBuffer(count, size, is32BitIndex);

    // 设置堆属性
    D3D12_HEAP_PROPERTIES heapProps = {};
    if (isStatic) 
    {
        // 静态缓冲区使用DEFAULT堆，适合GPU读取
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
    }
    else
    {
        // 动态缓冲区使用UPLOAD堆，适合CPU写入GPU读取
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
    }

    // 设置初始状态
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    // 创建底层D3D12资源
    ComPtr<ID3D12Resource> d3d12Resource = CreateBuffer(
        size,
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        initialState
    );

    // 设置原生资源
    indexBuffer->SetNativeResource(d3d12Resource.Get());
    indexBuffer->SetResourceState(RALResourceState::IndexBuffer);

    return indexBuffer;
}

IRALConstBuffer* DX12RALDevice::CreateConstBuffer(uint32_t size)
{
    // 创建DX12RALConstBuffer对象
    DX12RALConstBuffer* constBuffer = new DX12RALConstBuffer(size);

    // 创建变换矩阵常量缓冲区
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> d3d12Resource;

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        return nullptr;
    }

    constBuffer->SetNativeResource(d3d12Resource.Get());
    constBuffer->SetResourceState(RALResourceState::VertexBuffer);

    return constBuffer;
}

bool DX12RALDevice::UploadBuffer(IRALBuffer* buffer, const char* data, uint64_t size)
{
    // 等待之前的复制操作完成
    WaitForPreviousOperations();

    // 创建上传堆
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // 创建上传缓冲区
    auto uploadBuffer = CreateBuffer(
        size,
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );

    // 记录下来到EndFrame之后释放
    m_uploadingResources.push_back(uploadBuffer);

    // 映射上传缓冲区并复制数据
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer->Map(0, &readRange, &mappedData);
    memcpy(mappedData, data, size);
    uploadBuffer->Unmap(0, nullptr);

    ID3D12GraphicsCommandList* commandList = (ID3D12GraphicsCommandList*)m_graphicsCommandList->GetNativeCommandList();

    RALResourceState oldState = buffer->GetResourceState();

    // 记录复制命令到命令列表
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = (ID3D12Resource*)buffer->GetNativeResource();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = ConvertToDX12ResourceState(oldState);
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

    commandList->ResourceBarrier(1, &barrier);

    // 复制数据
    commandList->CopyBufferRegion(
        (ID3D12Resource*)buffer->GetNativeResource(),
        0,
        uploadBuffer.Get(),
        0,
        size
    );

    // 转换缓冲区状态
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = ConvertToDX12ResourceState(oldState);

    commandList->ResourceBarrier(1, &barrier);

    return true;
}

IRALGraphicsCommandList* DX12RALDevice::GetGraphicsCommandList()
{
    return m_graphicsCommandList.Get();
}

// 清理资源
void DX12RALDevice::Cleanup()
{
    // 等待所有命令完成
    WaitForPreviousFrame();

    m_uploadingResources.clear();

    // 关闭围栏事件
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

// 创建渲染目标
IRALRenderTarget* DX12RALDevice::CreateRenderTarget(uint32_t width, uint32_t height, RALDataFormat format)
{
    // 创建DX12RALRenderTarget对象
    DX12RALRenderTarget* renderTarget = new DX12RALRenderTarget(width, height, format);

    // 定义纹理描述符
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = toDXGIFormat(format);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // 设置堆属性（使用默认堆，适合GPU读取）
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // 设置初始状态
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // 创建底层D3D12资源
    ComPtr<ID3D12Resource> d3d12Resource;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        delete renderTarget;
        return nullptr;
    }

    // 设置原生资源和描述符信息
    renderTarget->SetNativeResource(d3d12Resource.Get());
    renderTarget->SetResourceState(RALResourceState::RenderTarget);

    return renderTarget;
}

// 创建渲染目标视图
IRALRenderTargetView* DX12RALDevice::CreateRenderTargetView(IRALRenderTarget* renderTarget, const RALRenderTargetViewDesc& desc)
{
    if (!renderTarget)
    {
        return nullptr;
    }

    // 创建DX12RALRenderTargetView对象
    DX12RALRenderTargetView* rtv = new DX12RALRenderTargetView();

    // 分配RTV描述符
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    uint32_t rtvIndex;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;

    if (!m_RTVDescriptorHeaps.AllocateDescriptor(rtvHandle, rtvHeap, rtvIndex))
    {
        delete rtv;
        return nullptr;
    }

    // 设置渲染目标资源和设备信息
    rtv->SetRenderTarget(renderTarget);
    rtv->SetRTVHeap(rtvHeap.Get());
    rtv->SetRTVHandle(rtvHandle);
    rtv->SetRTVIndex(rtvIndex);
    rtv->SetDevice(this);

    // 从renderTarget获取原生资源
    DX12RALRenderTarget* dx12RenderTarget = static_cast<DX12RALRenderTarget*>(renderTarget);
    ID3D12Resource* d3d12Resource = static_cast<ID3D12Resource*>(dx12RenderTarget->GetNativeResource());

    if (!d3d12Resource)
    {
        delete rtv;
        return nullptr;
    }

    // 创建RTV
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    // 如果描述符中指定了格式，则使用指定的格式，否则使用渲染目标的格式
    rtvDesc.Format = desc.format != RALDataFormat::Undefined ? toDXGIFormat(desc.format) : toDXGIFormat(renderTarget->GetFormat());
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = desc.mipSlice;
    rtvDesc.Texture2D.PlaneSlice = desc.planeSlice;

    m_device->CreateRenderTargetView(d3d12Resource, &rtvDesc, rtvHandle);

    return rtv;
}

// 创建深度/模板缓冲区
IRALDepthStencil* DX12RALDevice::CreateDepthStencil(uint32_t width, uint32_t height, RALDataFormat format)
{
    // 创建DX12RALDepthStencil对象
    DX12RALDepthStencil* depthStencil = new DX12RALDepthStencil(width, height, format);

    // 定义纹理描述符
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = toDXGIFormat(format);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 设置堆属性（使用默认堆，适合GPU读取）
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // 设置初始状态
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    // 创建深度/模板资源描述符
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = isTypelessFormat(format) ? toDXGIFormat(getDepthStencilFormatFromTypeless(format)) : toDXGIFormat(format);
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // 创建底层D3D12资源
    ComPtr<ID3D12Resource> d3d12Resource;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        &clearValue,
        IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        delete depthStencil;
        return nullptr;
    }

    // 设置原生资源和描述符信息
    depthStencil->SetNativeResource(d3d12Resource.Get());
    depthStencil->SetResourceState(RALResourceState::DepthStencil);

    return depthStencil;
}

// 创建深度模板视图
IRALDepthStencilView* DX12RALDevice::CreateDepthStencilView(IRALDepthStencil* depthStencil, const RALDepthStencilViewDesc& desc)
{
    if (!depthStencil)
    {
        return nullptr;
    }

    DX12RALDepthStencil* dx12DepthStencil = static_cast<DX12RALDepthStencil*>(depthStencil);
    ID3D12Resource* d3d12Resource = static_cast<ID3D12Resource*>(dx12DepthStencil->GetNativeResource());
    
    // 创建一个新的深度模板视图
    DX12RALDepthStencilView* dsv = new DX12RALDepthStencilView();
    dsv->SetDepthStencil(depthStencil);
    dsv->SetDevice(this);

    // 分配DSV描述符
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
    uint32_t dsvIndex;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;

    if (!m_DSVDescriptorHeaps.AllocateDescriptor(dsvHandle, dsvHeap, dsvIndex))
    {
        delete dsv;
        return nullptr;
    }

    dsv->SetDSVHandle(dsvHandle);
    dsv->SetDSVHeap(dsvHeap.Get());
    dsv->SetDSVIndex(dsvIndex);

    // 创建DSV
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    
    // 确定使用的格式
    RALDataFormat formatToUse = desc.format != RALDataFormat::Undefined ? desc.format : depthStencil->GetFormat();
    
    // 对于无类型格式，需要转换为对应的深度模板格式
    dsvDesc.Format = isTypelessFormat(formatToUse) ? toDXGIFormat(getDepthStencilFormatFromTypeless(formatToUse)) : toDXGIFormat(formatToUse);
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = desc.mipSlice;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    
    m_device->CreateDepthStencilView(d3d12Resource, &dsvDesc, dsvHandle);
    
    return dsv;
}

// 创建着色器资源视图
IRALShaderResourceView* DX12RALDevice::CreateShaderResourceView(IRALResource* resource, const RALShaderResourceViewDesc& desc)
{
    if (!resource)
    {
        return nullptr;
    }

    // 创建一个新的着色器资源视图
    DX12RALShaderResourceView* srv = new DX12RALShaderResourceView();
    srv->SetResource(resource);
    srv->SetDevice(this);

    // 分配SRV描述符
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
    uint32_t srvIndex;
    ComPtr<ID3D12DescriptorHeap> srvHeap;

    if (!m_SRVDescriptorHeaps.AllocateDescriptor(srvHandle, srvHeap, srvIndex))
    {
        delete srv;
        return nullptr;
    }

    srv->SetSRVHandle(srvHandle);
    srv->SetSRVHeap(srvHeap.Get());
    srv->SetSRVIndex(srvIndex);

    // 获取原生资源
    ID3D12Resource* d3d12Resource = static_cast<ID3D12Resource*>(resource->GetNativeResource());
    if (!d3d12Resource)
    {
        delete srv;
        return nullptr;
    }

    // 创建SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    // 确定资源类型并设置相应的视图维度
    RALResourceType resourceType = resource->GetResourceType();
    
    // 这里简化处理，主要支持纹理类型的资源
    // 对于其他资源类型（如缓冲区），可以根据需要扩展
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    
    // 设置格式
    if (desc.format != RALDataFormat::Undefined)
    {
        srvDesc.Format = toDXGIFormat(desc.format);
    }
    else
    {
        // 定义一个辅助函数来从无类型格式获取着色器资源格式
        auto getShaderResourceFormatFromTypeless = [](DXGI_FORMAT format) -> DXGI_FORMAT {
            // 简单的格式映射，可根据需要扩展
            switch (format)
            {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case DXGI_FORMAT_R32G32B32_TYPELESS:
                return DXGI_FORMAT_R32G32B32_FLOAT;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R32G32_TYPELESS:
                return DXGI_FORMAT_R32G32_FLOAT;
            case DXGI_FORMAT_R16G16_TYPELESS:
                return DXGI_FORMAT_R16G16_FLOAT;
            case DXGI_FORMAT_R32_TYPELESS:
                return DXGI_FORMAT_R32_FLOAT;
            case DXGI_FORMAT_R16_TYPELESS:
                return DXGI_FORMAT_R16_FLOAT;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8_TYPELESS:
                return DXGI_FORMAT_R8G8_UNORM;
            case DXGI_FORMAT_R8_TYPELESS:
                return DXGI_FORMAT_R8_UNORM;
            default:
                return format; // 如果不是无类型格式，直接返回
            }
        };

        // 尝试根据资源类型获取格式
        if (resourceType == RALResourceType::RenderTarget)
        {
            srvDesc.Format = toDXGIFormat(static_cast<IRALRenderTarget*>(resource)->GetFormat());
        }
        else if (resourceType == RALResourceType::DepthStencil)
        {
            // 对于深度模板资源，需要获取对应的SRV格式
            RALDataFormat depthFormat = static_cast<IRALDepthStencil*>(resource)->GetFormat();
            srvDesc.Format = isTypelessFormat(depthFormat) ? getShaderResourceFormatFromTypeless(toDXGIFormat(depthFormat)) : toDXGIFormat(depthFormat);
        }
        else
        {
            // 如果无法确定格式，使用默认格式或抛出错误
            // 这里简化处理，实际项目中应该有更完善的错误处理
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        }
    }
    
    // 设置MIP和数组切片信息
    srvDesc.Texture2D.MostDetailedMip = desc.mostDetailedMip;
    srvDesc.Texture2D.MipLevels = desc.mipLevels;
    srvDesc.Texture2D.PlaneSlice = 0; // 单平面纹理
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    
    m_device->CreateShaderResourceView(d3d12Resource, &srvDesc, srvHandle);
    
    return srv;
}

// 释放渲染目标视图描述符
bool DX12RALDevice::ReleaseRTVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t index, ID3D12DescriptorHeap* heap)
{
    return m_RTVDescriptorHeaps.FreeDescriptor(heap, index);
}

// 释放深度模板视图描述符
bool DX12RALDevice::ReleaseDSVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t index, ID3D12DescriptorHeap* heap)
{
    return m_DSVDescriptorHeaps.FreeDescriptor(heap, index);
}

// 释放着色器资源视图描述符
bool DX12RALDevice::ReleaseSRVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t index, ID3D12DescriptorHeap* heap)
{
    return m_SRVDescriptorHeaps.FreeDescriptor(heap, index);
}
