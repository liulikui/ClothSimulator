#include "DX12Renderer.h"
#include "Scene.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <d3dcompiler.h>
#ifdef _DEBUG
#include <d3d12sdklayers.h> // 包含D3D12调试层相关的定义
#endif

#include "DX12RALResource.h"

// 日志函数声明
extern void logDebug(const std::string& message);

// 定义缺失的常量
#define D3D12_SIMULTANEOUS_RENDERTARGET_COUNT 8

// 定义一些常量
const uint32_t kDefaultFrameCount = 2;

// 构造函数
DX12Renderer::DX12Renderer(uint32_t width, uint32_t height, const std::wstring& windowName, HWND hWnd)
    : m_width(width), m_height(height), m_windowName(windowName), m_backBufferCount(kDefaultFrameCount), m_hWnd(hWnd),
      m_fenceValue(0), m_fenceEvent(nullptr), m_currentFrameIndex(0)
{
    // 相机现在在Main.cpp中初始化
}

// 析构函数
DX12Renderer::~DX12Renderer()
{
    Cleanup();
}

// 初始化DirectX 12
bool DX12Renderer::Initialize()
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
    CreateRenderTargetViews();

    // 创建深度/模板视图
    CreateDepthStencilView();

    // 创建根签名
    CreateRootSignature();

    // 创建管道状态对象
    CreatePipelineStateObjects();

    // 创建材质缓冲区
    if (!CreateMaterialBuffer())
    {
        return false;
    }
    
    return true;
}

// 更新光源位置
void DX12Renderer::UpdateLightPosition(const dx::XMFLOAT4& position)
{
    m_lightPosition = position;
}

// 更新光源颜色
void DX12Renderer::UpdateLightColor(const dx::XMFLOAT4& color)
{
    m_lightColor = color;
}

// 创建设备和交换链
bool DX12Renderer::CreateDeviceAndSwapChain()
{
    // 在调试版本中启用D3D12调试层
#ifdef _DEBUG
    TSharePtr<ID3D12Debug> debugController;
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
    TSharePtr<IDXGIAdapter1> adapter;
    IDXGIAdapter1* adapterPtr = nullptr;
    for (uint32_t adapterIndex = 0; !foundAdapter && DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapterPtr); ++adapterIndex)
    {
        adapter = TSharePtr<IDXGIAdapter1>(adapterPtr);
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
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // 创建交换链
    TSharePtr<IDXGISwapChain1> swapChain1;
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

    return true;
}

// 创建命令对象
void DX12Renderer::CreateCommandObjects()
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

    // 创建命令列表
    HRESULT hr = m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[0].Get(), // 初始使用第一个命令分配器
        nullptr,
        IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create command list.");
    }

    // 关闭命令列表（初始状态是打开的）
    m_commandList->Close();
}

// 创建描述符堆
void DX12Renderer::CreateDescriptorHeaps()
{
    // 获取描述符大小
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 创建渲染目标视图堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = m_backBufferCount;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create RTV heap.");
    }

    // 创建深度/模板视图堆
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create DSV heap.");
    }

    // 创建着色器资源视图堆
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 10;  // 预留一些描述符
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_srvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create SRV heap.");
    }
}

// 创建渲染目标视图
void DX12Renderer::CreateRenderTargetViews()
{
    // 获取渲染目标视图堆的起始句柄
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

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
void DX12Renderer::CreateDepthStencilView()
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

    // 创建深度/模板缓冲区
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        nullptr,
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
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );
}

// 设置根签名
void DX12Renderer::SetRootSignature(TSharePtr<IRALRootSignature> rootSignature)
{
    m_rootSignature = rootSignature;
    logDebug("[DEBUG] DX12Renderer::SetRootSignature called and stored");
}

// 通用着色器编译辅助方法
ComPtr<ID3DBlob> DX12Renderer::CompileShaderBlob(const char* shaderCode, const char* entryPoint, const char* target)
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
IRALVertexShader* DX12Renderer::CompileVertexShader(const char* shaderCode, const char* entryPoint)
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
IRALPixelShader* DX12Renderer::CompilePixelShader(const char* shaderCode, const char* entryPoint)
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
IRALGeometryShader* DX12Renderer::CompileGeometryShader(const char* shaderCode, const char* entryPoint)
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
IRALComputeShader* DX12Renderer::CompileComputeShader(const char* shaderCode, const char* entryPoint)
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
IRALMeshShader* DX12Renderer::CompileMeshShader(const char* shaderCode, const char* entryPoint)
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
IRALAmplificationShader* DX12Renderer::CompileAmplificationShader(const char* shaderCode, const char* entryPoint)
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
IRALRayGenShader* DX12Renderer::CompileRayGenShader(const char* shaderCode, const char* entryPoint)
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
IRALRayMissShader* DX12Renderer::CompileRayMissShader(const char* shaderCode, const char* entryPoint)
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
IRALRayHitGroupShader* DX12Renderer::CompileRayHitGroupShader(const char* shaderCode, const char* entryPoint)
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
IRALRayCallableShader* DX12Renderer::CompileRayCallableShader(const char* shaderCode, const char* entryPoint)
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

// 创建并获取根签名
TSharePtr<IRALRootSignature> DX12Renderer::CreateAndGetRootSignature()
{
    logDebug("[DEBUG] DX12Renderer::CreateAndGetRootSignature called");
    
    // 定义根参数
    std::vector<D3D12_ROOT_PARAMETER> rootParameters(2);

    // 根参数0: 常量缓冲区视图（变换矩阵）
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;

    // 根参数1: 常量缓冲区视图（材质和光照）
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].Descriptor.RegisterSpace = 0;

    // 定义静态采样器
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 创建根签名描述
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = static_cast<uint32_t>(rootParameters.size());
    rootSignatureDesc.pParameters = rootParameters.data();
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // 序列化根签名
    TSharePtr<ID3DBlob> rootSignatureBlob;
    TSharePtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,
        rootSignatureBlob.ReleaseAndGetAddressOf(),
        errorBlob.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string errorMsg = "Failed to serialize root signature: " + std::string(static_cast<char*>(errorBlob->GetBufferPointer()));
            logDebug("[DEBUG] " + errorMsg);
            std::cerr << errorMsg << std::endl;
        }
        logDebug("[DEBUG] Failed to serialize root signature");
        return TSharePtr<IRALRootSignature>();
    }

    // 创建根签名
    TSharePtr<ID3D12RootSignature> d3d12RootSignature;
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
        return TSharePtr<IRALRootSignature>();
    }
    
    logDebug("[DEBUG] Root signature created successfully");
    
    // 创建并返回IRALRootSignature对象
    auto ralRootSignature = new DX12RALRootSignature();
    static_cast<DX12RALRootSignature*>(ralRootSignature)->SetNativeRootSignature(d3d12RootSignature.Get());
    
    return TSharePtr<IRALRootSignature>(ralRootSignature);
}

// 创建根签名（旧方法，保持兼容性，但现在会调用新方法）
void DX12Renderer::CreateRootSignature()
{
    logDebug("[DEBUG] DX12Renderer::CreateRootSignature called (deprecated, use CreateAndGetRootSignature instead)");
    
    // 由于根签名现在由Scene管理，这个方法可能不再需要
    // 但为了保持兼容性，我们仍然实现它
    // 这里我们可以选择调用新方法并忽略结果，或者保留旧的实现
    
    // 保留旧的实现，以便向后兼容
    // 定义根参数
    std::vector<D3D12_ROOT_PARAMETER> rootParameters(2);

    // 根参数0: 常量缓冲区视图（变换矩阵）
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;

    // 根参数1: 常量缓冲区视图（材质和光照）
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].Descriptor.RegisterSpace = 0;

    // 定义静态采样器
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 创建根签名描述
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = static_cast<uint32_t>(rootParameters.size());
    rootSignatureDesc.pParameters = rootParameters.data();
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // 序列化根签名
    TSharePtr<ID3DBlob> rootSignatureBlob;
    TSharePtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,
        rootSignatureBlob.ReleaseAndGetAddressOf(),
        errorBlob.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string errorMsg = "Failed to serialize root signature: " + std::string(static_cast<char*>(errorBlob->GetBufferPointer()));
            logDebug("[DEBUG] " + errorMsg);
            std::cerr << errorMsg << std::endl;
        }
        logDebug("[DEBUG] Failed to serialize root signature");
        throw std::runtime_error("Failed to serialize root signature.");
    }

    // 创建根签名
    TSharePtr<ID3D12RootSignature> d3d12RootSignature;
    hr = m_device->CreateRootSignature(
        0,
        rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(d3d12RootSignature.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        std::cerr << "Failed to create root signature. HRESULT: " << hr << std::endl;
        throw std::runtime_error("Failed to create root signature.");
    }

    logDebug("[DEBUG] Root signature created in CreateRootSignature");
    
    // 创建并设置IRALRootSignature对象
    auto ralRootSignature = new DX12RALRootSignature();
    static_cast<DX12RALRootSignature*>(ralRootSignature)->SetNativeRootSignature(d3d12RootSignature.Get());
    SetRootSignature(TSharePtr<IRALRootSignature>(ralRootSignature));
}

// 创建管道状态对象
void DX12Renderer::CreatePipelineStateObjects()
{
    // 顶点输入布局
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // 使用内联着色器代码作为简化示例
    // 在实际应用中，应该从文件加载HLSL着色器代码并编译
    const char* vertexShaderCode = ""
        "struct VS_INPUT {\n"
        "    float3 pos : POSITION;\n"
        "    float3 normal : NORMAL;\n"
        "};\n"
        "struct VS_OUTPUT {\n"
        "    float4 pos : SV_POSITION;\n"
        "    float3 normal : NORMAL;\n"
        "    float3 worldPos : WORLD_POS;\n"
        "};\n"
        "cbuffer ConstantBuffer : register(b0) {\n"
        "    float4x4 worldViewProj;\n"
        "    float4x4 world;\n"
        "};\n"
        "VS_OUTPUT main(VS_INPUT input) {\n"
        "    VS_OUTPUT output;\n"
        "    output.pos = mul(float4(input.pos, 1.0f), worldViewProj);\n"
        "    output.worldPos = mul(float4(input.pos, 1.0f), world).xyz;\n"
        "    // Directly transform normal with world matrix since we don't have non-uniform scaling\n"
        "    float3x3 worldMat = (float3x3)world;\n"
        "    output.normal = mul(input.normal, worldMat);\n"
        "    // Ensure normal is normalized for correct lighting\n"
        "    output.normal = normalize(output.normal);\n"
        "    return output;\n"
        "}";

    const char* pixelShaderCode = ""
        "struct PS_INPUT {\n"
        "    float4 pos : SV_POSITION;\n"
        "    float3 normal : NORMAL;\n"
        "    float3 worldPos : WORLD_POS;\n"
        "};\n"
        "cbuffer MaterialBuffer : register(b0) {\n"
        "    float4 diffuseColor;\n"
        "    float3 lightPos;\n"
        "    float3 lightColor;\n"
        "};\n"
        "float4 main(PS_INPUT input) : SV_TARGET {\n"
        "    float3 normal = normalize(input.normal);\n"
        "    float3 lightDir = normalize(lightPos - input.worldPos);\n"
        "    float3 viewDir = normalize(float3(0, 0, 5) - input.worldPos);\n"
        "    float3 halfVec = normalize(lightDir + viewDir);\n"
        "    \n"
        "    float diffuse = max(dot(normal, lightDir), 0.0f);\n"
        "    float specular = pow(max(dot(normal, halfVec), 0.0f), 32.0f);\n"
        "    \n"
        "    float3 ambient = float3(0.2f, 0.2f, 0.2f);\n"
        "    float3 finalColor = ambient + diffuse * diffuseColor.rgb * lightColor + specular * lightColor;\n"
        "    \n"
        "    return float4(finalColor, diffuseColor.a);\n"
        "}";

    // 编译顶点着色器
    TSharePtr<ID3DBlob> vertexShaderBlob;
    TSharePtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(
        vertexShaderCode,
        strlen(vertexShaderCode),
        "VertexShader",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "vs_5_0",
        0,
        0,
        vertexShaderBlob.ReleaseAndGetAddressOf(),
        errorBlob.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::cerr << "Failed to compile vertex shader: " << static_cast<char*>(errorBlob->GetBufferPointer()) << std::endl;
        }
        throw std::runtime_error("Failed to compile vertex shader.");
    }

    // 编译像素着色器
    TSharePtr<ID3DBlob> pixelShaderBlob;
    hr = D3DCompile(
        pixelShaderCode,
        strlen(pixelShaderCode),
        "PixelShader",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "ps_5_0",
        0,
        0,
        pixelShaderBlob.ReleaseAndGetAddressOf(),
        errorBlob.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::cerr << "Failed to compile pixel shader: " << static_cast<char*>(errorBlob->GetBufferPointer()) << std::endl;
        }
        throw std::runtime_error("Failed to compile pixel shader.");
    }

    // 图形管道状态描述
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, ARRAYSIZE(inputLayout) };
    if (m_rootSignature) {
        psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(m_rootSignature->GetNativeResource());
    }
    psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // 使用原生的D3D12结构体代替CD3D12便利类
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 关闭剔除，启用双面渲染
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.RasterizerState = rasterizerDesc;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = 
    {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };
    for (uint32_t i = 0; i < D3D12_SIMULTANEOUS_RENDERTARGET_COUNT; ++i)
    {
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }
    psoDesc.BlendState = blendDesc;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = 
    {
        D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS
    };
    depthStencilDesc.FrontFace = defaultStencilOp;
    depthStencilDesc.BackFace = defaultStencilOp;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.SampleMask = UINT32_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    // 创建布料管道状态对象
    hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_clothPipelineState.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create cloth pipeline state object.");
    }

    // 创建球体管道状态对象（与布料相同，但可以根据需要修改）
    hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_spherePipelineState.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create sphere pipeline state object.");
    }
}

// 创建缓冲区
TSharePtr<ID3D12Resource> DX12Renderer::CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
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

    TSharePtr<ID3D12Resource> buffer;
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

// 上传资源数据
template<typename T>
void DX12Renderer::UploadBufferData(TSharePtr<ID3D12Resource>& buffer, const std::vector<T>& data)
{
    // 创建上传堆
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // 创建上传缓冲区
    auto uploadBuffer = CreateBuffer(
        data.size() * sizeof(T),
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );

    // 映射上传缓冲区并复制数据
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer->Map(0, &readRange, &mappedData);
    memcpy(mappedData, data.data(), data.size() * sizeof(T));
    uploadBuffer->Unmap(0, nullptr);

    // 创建一个临时的命令分配器和命令列表来执行复制操作
    // 这样就不会影响主命令列表的状态
    TSharePtr<ID3D12CommandAllocator> tempCommandAllocator;
    TSharePtr<ID3D12GraphicsCommandList> tempCommandList;
    
    // 创建临时命令分配器
    HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(tempCommandAllocator.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create temporary command allocator.");
    }
    
    // 创建临时命令列表
    hr = m_device->CreateCommandList(0,
                                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                                   tempCommandAllocator.Get(),
                                   nullptr,
                                   IID_PPV_ARGS(tempCommandList.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create temporary command list.");
    }

    // 记录复制命令到临时命令列表
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = buffer.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

    tempCommandList->ResourceBarrier(1, &barrier);

    // 复制数据
    tempCommandList->CopyBufferRegion(
        buffer.Get(),
        0,
        uploadBuffer.Get(),
        0,
        data.size() * sizeof(T)
    );

    // 转换缓冲区状态为GENERIC_READ，这是一个通用状态，可以被用作顶点、索引或常量缓冲区
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

    tempCommandList->ResourceBarrier(1, &barrier);

    // 关闭并执行临时命令列表
    tempCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { tempCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 等待命令执行完成
    WaitForPreviousFrame();
}

// 创建材质缓冲区
bool DX12Renderer::CreateMaterialBuffer()
{
    // 创建材质缓冲区描述
    const UINT materialBufferSize = sizeof(MaterialBuffer);

    // 创建材质和光照常量缓冲区
    try
    {
        m_materialBuffer = CreateBuffer(
            materialBufferSize,
            D3D12_RESOURCE_FLAG_NONE,
            { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 },
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to create material buffer: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// 更新材质和光照常量缓冲区
void DX12Renderer::UpdateMaterialBuffer(const dx::XMFLOAT4& diffuseColor)
{
    // 准备材质缓冲区数据
    MaterialBuffer data;
    data.diffuseColor = diffuseColor;
    // 将XMFLOAT4转换为XMFLOAT3，只使用前三个分量
    data.lightPos.x = m_lightPosition.x;
    data.lightPos.y = m_lightPosition.y;
    data.lightPos.z = m_lightPosition.z;
    data.padding1 = 0.0f;
    // 将XMFLOAT4转换为XMFLOAT3，只使用前三个分量
    data.lightColor.x = m_lightColor.x;
    data.lightColor.y = m_lightColor.y;
    data.lightColor.z = m_lightColor.z;
    data.padding2 = 0.0f;

    // 映射并更新缓冲区
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    m_materialBuffer->Map(0, &readRange, &mappedData);
    memcpy(mappedData, &data, sizeof(MaterialBuffer));
    m_materialBuffer->Unmap(0, nullptr);

    // 设置根参数1（材质和光照常量缓冲区）
    m_commandList->SetGraphicsRootConstantBufferView(1, m_materialBuffer->GetGPUVirtualAddress());
}

// 渲染单个Primitive对象
// 注意：这里不再需要viewMatrix和projectionMatrix参数，因为它们已经存储在Scene的cameraConstBuffer中
void DX12Renderer::RenderPrimitive(const dx::XMMATRIX& worldMatrix, const dx::XMFLOAT4& diffuseColor, bool isCloth) {
    logDebug("[DEBUG] RenderPrimitive called, isCloth: " + std::string(isCloth ? "true" : "false"));
    
    // 检查commandList是否有效
    if (!m_commandList || !m_commandQueue || !m_swapChain) {
        logDebug("[DEBUG] RenderPrimitive: Missing required components");
        return;
    }
    
    // 设置正确的管道状态
    if (isCloth && m_clothPipelineState) {
        logDebug("[DEBUG] Using cloth pipeline state");
        m_commandList->SetPipelineState(m_clothPipelineState.Get());
    } else if (m_spherePipelineState) {
        logDebug("[DEBUG] Using sphere pipeline state");
        m_commandList->SetPipelineState(m_spherePipelineState.Get());
    } else {
        logDebug("[DEBUG] No valid pipeline state available");
        // 如果都没有，返回
        return;
    }

    // 更新材质
    logDebug("[DEBUG] Updating material buffer");
    UpdateMaterialBuffer(diffuseColor);
    
    // 创建世界矩阵缓冲区数据
    struct WorldMatrixBuffer
    {
        dx::XMFLOAT4X4 WorldMatrix;
    } worldData;
    dx::XMStoreFloat4x4(&worldData.WorldMatrix, dx::XMMatrixTranspose(worldMatrix));
    
    // 创建一个临时的常量缓冲区用于存储世界矩阵
    TSharePtr<ID3D12Resource> worldMatrixBuffer;
    try
    {
        worldMatrixBuffer = CreateBuffer(
            sizeof(WorldMatrixBuffer),
            D3D12_RESOURCE_FLAG_NONE,
            { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 },
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
        
        // 更新世界矩阵缓冲区
        void* mappedData = nullptr;
        D3D12_RANGE readRange = {0, 0};
        if (SUCCEEDED(worldMatrixBuffer->Map(0, &readRange, &mappedData)))
        {
            memcpy(mappedData, &worldData, sizeof(WorldMatrixBuffer));
            worldMatrixBuffer->Unmap(0, nullptr);
        }
        
        // 设置根参数0（世界矩阵）
        m_commandList->SetGraphicsRootConstantBufferView(0, worldMatrixBuffer->GetGPUVirtualAddress());
    }
    catch (const std::exception& e)
    {
        logDebug("[DEBUG] Failed to create world matrix buffer: " + std::string(e.what()));
        return;
    }
    
    // 设置顶点和索引缓冲区
    if (isCloth) {
        logDebug("[DEBUG] Cloth buffers - vertexBuffer: " + std::to_string(reinterpret_cast<uintptr_t>(m_clothVertexBuffer.Get())) + 
                ", indexBuffer: " + std::to_string(reinterpret_cast<uintptr_t>(m_clothIndexBuffer.Get())) + 
                ", vertexCount: " + std::to_string(m_clothVertexCount) + 
                ", indexCount: " + std::to_string(m_clothIndexCount));
        
        if (m_clothVertexBuffer && m_clothIndexBuffer && m_clothVertexCount > 0 && m_clothIndexCount > 0) {
            // 布料顶点缓冲区应该已经是正确的状态，不需要转换

            // 布料索引缓冲区应该已经是正确的状态，不需要转换

            logDebug("[DEBUG] Setting cloth vertex buffer view");
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
            vertexBufferView.BufferLocation = m_clothVertexBuffer->GetGPUVirtualAddress();
            vertexBufferView.StrideInBytes = sizeof(dx::XMFLOAT3) * 2;  // 位置 + 法线
            vertexBufferView.SizeInBytes = m_clothVertexCount * sizeof(dx::XMFLOAT3) * 2;
            m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

            logDebug("[DEBUG] Setting cloth index buffer view");
            D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
            indexBufferView.BufferLocation = m_clothIndexBuffer->GetGPUVirtualAddress();
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = m_clothIndexCount * sizeof(uint32_t);
            m_commandList->IASetIndexBuffer(&indexBufferView);

            // 设置图元拓扑
            logDebug("[DEBUG] Setting primitive topology");
            m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 绘制
            logDebug("[DEBUG] Drawing cloth with " + std::to_string(m_clothIndexCount) + " indices");
            m_commandList->DrawIndexedInstanced(m_clothIndexCount, 1, 0, 0, 0);
        } else {
            logDebug("[DEBUG] Cloth buffers not ready");
        }
    } else {
        logDebug("[DEBUG] Sphere buffers - vertexBuffer: " + std::to_string(reinterpret_cast<uintptr_t>(m_sphereVertexBuffer.Get())) + 
                ", indexBuffer: " + std::to_string(reinterpret_cast<uintptr_t>(m_sphereIndexBuffer.Get())) + 
                ", vertexCount: " + std::to_string(m_sphereVertexCount) + 
                ", indexCount: " + std::to_string(m_sphereIndexCount));
        
        if (m_sphereVertexBuffer && m_sphereIndexBuffer && m_sphereVertexCount > 0 && m_sphereIndexCount > 0) {
            // 转换球体顶点缓冲区状态
            logDebug("[DEBUG] Transitioning sphere vertex buffer state");
            D3D12_RESOURCE_BARRIER vertexBarrier = {};
            vertexBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            vertexBarrier.Transition.pResource = m_sphereVertexBuffer.Get();
            vertexBarrier.Transition.Subresource = 0;
            vertexBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
            vertexBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            m_commandList->ResourceBarrier(1, &vertexBarrier);

            // 转换球体索引缓冲区状态
            logDebug("[DEBUG] Transitioning sphere index buffer state");
            D3D12_RESOURCE_BARRIER indexBarrier = {};
            indexBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            indexBarrier.Transition.pResource = m_sphereIndexBuffer.Get();
            indexBarrier.Transition.Subresource = 0;
            indexBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
            indexBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
            m_commandList->ResourceBarrier(1, &indexBarrier);

            logDebug("[DEBUG] Setting sphere vertex buffer view");
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
            vertexBufferView.BufferLocation = m_sphereVertexBuffer->GetGPUVirtualAddress();
            vertexBufferView.StrideInBytes = sizeof(dx::XMFLOAT3) * 2;  // 位置 + 法线
            vertexBufferView.SizeInBytes = m_sphereVertexCount * sizeof(dx::XMFLOAT3) * 2;
            m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

            logDebug("[DEBUG] Setting sphere index buffer view");
            D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
            indexBufferView.BufferLocation = m_sphereIndexBuffer->GetGPUVirtualAddress();
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = m_sphereIndexCount * sizeof(uint32_t);
            m_commandList->IASetIndexBuffer(&indexBufferView);

            // 设置图元拓扑
            logDebug("[DEBUG] Setting primitive topology");
            m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 绘制
            logDebug("[DEBUG] Drawing sphere with " + std::to_string(m_sphereIndexCount) + " indices");
            m_commandList->DrawIndexedInstanced(m_sphereIndexCount, 1, 0, 0, 0);
        } else {
            logDebug("[DEBUG] Sphere buffers not ready");
        }
    }
    logDebug("[DEBUG] RenderPrimitive finished");
}

// 清理资源


// 渲染一帧
void DX12Renderer::Render(Scene* scene, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    // 获取当前后台缓冲区
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_currentBackBufferIndex * m_rtvDescriptorSize;

    // 重置当前帧的命令分配器
    HRESULT hr = m_commandAllocators[m_currentFrameIndex]->Reset();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command allocator.");
    }

    // 重置命令列表并关联当前帧的命令分配器
    hr = m_commandList->Reset(m_commandAllocators[m_currentFrameIndex].Get(), nullptr);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command list.");
    }

    // 设置根签名
    if (m_rootSignature) {
        ID3D12RootSignature* nativeRootSignature = static_cast<ID3D12RootSignature*>(m_rootSignature->GetNativeResource());
        m_commandList->SetGraphicsRootSignature(nativeRootSignature);
    }

    // 资源转换：设置渲染目标为渲染状态
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_backBuffers[m_currentBackBufferIndex].Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    m_commandList->ResourceBarrier(1, &barrier);

    // 清除渲染目标和深度缓冲区 - 浅灰色背景
    const float clearColor[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr
    );

    // 设置渲染目标和深度/模板视图
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // 设置视口和裁剪矩形
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_commandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(m_width);
    scissorRect.bottom = static_cast<LONG>(m_height);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    // 调用场景的render方法执行实际的渲染逻辑
    if (scene) {        
        logDebug("[DEBUG] Render: Calling scene->render()");
        
        // 让scene自己处理相机常量缓冲区的创建和更新
        scene->render(this, viewMatrix, projectionMatrix);
    } else {
        logDebug("[DEBUG] Render: WARNING - Scene is null!");
    }

    // 资源转换：设置渲染目标为呈现状态
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    m_commandList->ResourceBarrier(1, &barrier);

    // 关闭命令列表
    m_commandList->Close();

    // 执行命令列表
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 呈现
    HRESULT presentHr = m_swapChain->Present(1, 0);
    if (FAILED(presentHr))
    {
        throw std::runtime_error("Failed to present swap chain.");
    }

    // 等待当前帧完成
    WaitForPreviousFrame();
}

// 设置布料顶点数据
void DX12Renderer::SetClothVertices(const std::vector<dx::XMFLOAT3>& positions, const std::vector<dx::XMFLOAT3>& normals,
                                  const std::vector<uint32_t>& indices)
{
    std::cout << "DX12Renderer::SetClothVertices called with " << positions.size() << " positions, " << normals.size() << " normals, " << indices.size() << " indices" << std::endl;
    
    // 保存布料顶点和索引数量
    m_clothVertexCount = positions.size();
    m_clothIndexCount = indices.size();
    
    // 创建布料顶点数据
    std::vector<uint8_t> vertexData(m_clothVertexCount * sizeof(dx::XMFLOAT3) * 2); // 位置 + 法线
    for (size_t i = 0; i < m_clothVertexCount; ++i)
    {
        size_t positionOffset = i * sizeof(dx::XMFLOAT3) * 2;
        size_t normalOffset = positionOffset + sizeof(dx::XMFLOAT3);
        
        memcpy(&vertexData[positionOffset], &positions[i], sizeof(dx::XMFLOAT3));
        memcpy(&vertexData[normalOffset], &normals[i], sizeof(dx::XMFLOAT3));
    }
    
    // 上传顶点和索引数据
    UploadBufferData(m_clothVertexBuffer, vertexData);
    UploadBufferData(m_clothIndexBuffer, indices);
}


// 设置球体顶点数据
void DX12Renderer::SetSphereVertices(const std::vector<dx::XMFLOAT3>& positions, const std::vector<dx::XMFLOAT3>& normals,
                                   const std::vector<uint32_t>& indices)
{
    // 保存球体顶点和索引数量
    m_sphereVertexCount = positions.size();
    m_sphereIndexCount = indices.size();
    
    // 创建球体顶点数据
    std::vector<uint8_t> vertexData(m_sphereVertexCount * sizeof(dx::XMFLOAT3) * 2); // 位置 + 法线
    for (size_t i = 0; i < m_sphereVertexCount; ++i)
    {
        size_t positionOffset = i * sizeof(dx::XMFLOAT3) * 2;
        size_t normalOffset = positionOffset + sizeof(dx::XMFLOAT3);
        
        memcpy(&vertexData[positionOffset], &positions[i], sizeof(dx::XMFLOAT3));
        memcpy(&vertexData[normalOffset], &normals[i], sizeof(dx::XMFLOAT3));
    }
    
    // 上传顶点和索引数据
    UploadBufferData(m_sphereVertexBuffer, vertexData);
    UploadBufferData(m_sphereIndexBuffer, indices);
}


// 等待前一帧完成
void DX12Renderer::WaitForPreviousFrame()
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

    // 切换到下一帧的命令分配器
    m_currentFrameIndex = (m_currentFrameIndex + 1) % 2;
}

// 调整窗口大小
void DX12Renderer::Resize(uint32_t width, uint32_t height)
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
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to resize swap chain buffers.");
    }

    // 重新创建渲染目标视图
    CreateRenderTargetViews();

    // 重新创建深度/模板视图
    CreateDepthStencilView();

    // 注意：相机调整现在由Main.cpp处理
}

// 清理资源
void DX12Renderer::Cleanup()
{
    // 等待所有命令完成
    WaitForPreviousFrame();

    // 关闭围栏事件
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    // 释放资源（智能指针会自动处理）
}

// 创建图形管线状态
TSharePtr<IRALGraphicsPipelineState> DX12Renderer::CreateGraphicsPipelineState(const RALGraphicsPipelineStateDesc& desc)
{
    // 创建D3D12图形管线状态描述
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    
    // 设置输入布局
    if (desc.inputLayout) {
        // 假设inputLayout是std::vector<RALVertexAttribute>*
        std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputLayout;
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
        psoDesc.RTVFormats[i] = static_cast<DXGI_FORMAT>(desc.renderTargetFormats[i]);
    }
    
    psoDesc.DSVFormat = static_cast<DXGI_FORMAT>(desc.depthStencilFormat);
    
    // 设置采样描述
    psoDesc.SampleDesc.Count = desc.sampleDesc.Count;
    psoDesc.SampleDesc.Quality = desc.sampleDesc.Quality;
    
    // 创建D3D12管线状态对象
    TSharePtr<ID3D12PipelineState> pipelineState;
    HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create graphics pipeline state object.");
    }
    
    // 创建并返回RAL图形管线状态对象
    auto ralPipelineState = new DX12RALGraphicsPipelineState();
    ralPipelineState->SetNativePipelineState(pipelineState.Get());
    
    return TSharePtr<IRALGraphicsPipelineState>(ralPipelineState);
}