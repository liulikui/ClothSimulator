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

// 定义缺失的常量
#define D3D12_SIMULTANEOUS_RENDERTARGET_COUNT 8

// 定义一些常量
const uint32_t kDefaultFrameCount = 2;

// 构造函数
DX12Renderer::DX12Renderer(uint32_t width, uint32_t height, const std::wstring& windowName, HWND hWnd)
    : width_(width), height_(height), windowName_(windowName), backBufferCount_(kDefaultFrameCount), hWnd_(hWnd),
      fenceValue_(0), fenceEvent_(nullptr), currentFrameIndex_(0)
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

    // 创建常量缓冲区
    if (!CreateConstantBuffers())
    {
        return false;
    }
    
    return true;
}

// 更新光源位置
void DX12Renderer::UpdateLightPosition(const dx::XMFLOAT4& position)
{
    lightPosition_ = position;
}

// 更新光源颜色
void DX12Renderer::UpdateLightColor(const dx::XMFLOAT4& color)
{
    lightColor_ = color;
}

// 创建设备和交换链
bool DX12Renderer::CreateDeviceAndSwapChain()
{
    // 在调试版本中启用D3D12调试层
#ifdef _DEBUG
    ID3D12Debug* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif

    // 创建DXGI工厂
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(factory_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create DXGI factory." << std::endl;
        return false;
    }

    // 查找支持DirectX 12的GPU
    bool foundAdapter = false;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (uint32_t adapterIndex = 0; !foundAdapter && DXGI_ERROR_NOT_FOUND != factory_->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // 跳过软件适配器
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            // 尝试创建设备
            hr = D3D12CreateDevice(
                adapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(device_.ReleaseAndGetAddressOf())
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
    hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create command queue." << std::endl;
        return false;
    }

    // 创建交换链描述
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width_;
    swapChainDesc.Height = height_;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = backBufferCount_;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // 创建交换链
    wrl::ComPtr<IDXGISwapChain1> swapChain1;
    hr = factory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),
        hWnd_,  // 使用传入的窗口句柄
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
    hr = swapChain1.As(&swapChain_);
    if (FAILED(hr))
    {
        std::cerr << "Failed to upgrade swap chain to IDXGISwapChain4." << std::endl;
        return false;
    }

    // 获取当前后台缓冲区索引
    currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

    // 创建围栏用于同步
    hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create fence." << std::endl;
        return false;
    }

    // 创建围栏事件
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fenceEvent_)
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
        HRESULT hr = device_->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(commandAllocators_[i].ReleaseAndGetAddressOf())
        );

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command allocator.");
        }
    }

    // 创建命令列表
    HRESULT hr = device_->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocators_[0].Get(), // 初始使用第一个命令分配器
        nullptr,
        IID_PPV_ARGS(commandList_.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create command list.");
    }

    // 关闭命令列表（初始状态是打开的）
    commandList_->Close();
}

// 创建描述符堆
void DX12Renderer::CreateDescriptorHeaps()
{
    // 获取描述符大小
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    srvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 创建渲染目标视图堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = backBufferCount_;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create RTV heap.");
    }

    // 创建深度/模板视图堆
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create DSV heap.");
    }

    // 创建着色器资源视图堆
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 10;  // 预留一些描述符
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(srvHeap_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create SRV heap.");
    }
}

// 创建渲染目标视图
void DX12Renderer::CreateRenderTargetViews()
{
    // 获取渲染目标视图堆的起始句柄
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    // 为每个后台缓冲区创建渲染目标视图
    backBuffers_.resize(backBufferCount_);
    for (uint32_t i = 0; i < backBufferCount_; ++i)
    {
        // 获取后台缓冲区
        HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(backBuffers_[i].ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to get swap chain buffer.");
        }

        // 创建渲染目标视图
        device_->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);

        // 移动到下一个描述符
        rtvHandle.ptr += rtvDescriptorSize_;
    }
}

// 创建深度/模板视图
void DX12Renderer::CreateDepthStencilView()
{
    // 创建深度/模板缓冲区描述
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width_;
    depthStencilDesc.Height = height_;
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
    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        nullptr,
        IID_PPV_ARGS(depthStencilBuffer_.ReleaseAndGetAddressOf())
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

    device_->CreateDepthStencilView(
        depthStencilBuffer_.Get(),
        &dsvDesc,
        dsvHeap_->GetCPUDescriptorHandleForHeapStart()
    );
}

// 创建根签名
void DX12Renderer::CreateRootSignature()
{
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
    wrl::ComPtr<ID3DBlob> rootSignatureBlob;
    wrl::ComPtr<ID3DBlob> errorBlob;
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
            std::cerr << "Failed to serialize root signature: " << static_cast<char*>(errorBlob->GetBufferPointer()) << std::endl;
        }
        throw std::runtime_error("Failed to serialize root signature.");
    }

    // 创建根签名
    hr = device_->CreateRootSignature(
        0,
        rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(rootSignature_.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        std::cerr << "Failed to create root signature. HRESULT: " << hr << std::endl;
        throw std::runtime_error("Failed to create root signature.");
    }
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
    wrl::ComPtr<ID3DBlob> vertexShaderBlob;
    wrl::ComPtr<ID3DBlob> errorBlob;
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
    wrl::ComPtr<ID3DBlob> pixelShaderBlob;
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
    psoDesc.pRootSignature = rootSignature_.Get();
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
    hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(clothPipelineState_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create cloth pipeline state object.");
    }

    // 创建球体管道状态对象（与布料相同，但可以根据需要修改）
    hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(spherePipelineState_.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create sphere pipeline state object.");
    }
}

// 创建缓冲区
wrl::ComPtr<ID3D12Resource> DX12Renderer::CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
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

    wrl::ComPtr<ID3D12Resource> buffer;
    HRESULT hr = device_->CreateCommittedResource(
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
void DX12Renderer::UploadBufferData(wrl::ComPtr<ID3D12Resource>& buffer, const std::vector<T>& data)
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
    wrl::ComPtr<ID3D12CommandAllocator> tempCommandAllocator;
    wrl::ComPtr<ID3D12GraphicsCommandList> tempCommandList;
    
    // 创建临时命令分配器
    HRESULT hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(tempCommandAllocator.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create temporary command allocator.");
    }
    
    // 创建临时命令列表
    hr = device_->CreateCommandList(0,
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

    // 转换缓冲区状态
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

    tempCommandList->ResourceBarrier(1, &barrier);

    // 关闭并执行临时命令列表
    tempCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { tempCommandList.Get() };
    commandQueue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 等待命令执行完成
    WaitForPreviousFrame();
}

// 创建常量缓冲区
bool DX12Renderer::CreateConstantBuffers()
{
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
    desc.Width = sizeof(ConstantBuffer);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(constantBuffer_.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        std::cerr << "Failed to create constant buffer." << std::endl;
        return false;
    }

    // 创建材质和光照常量缓冲区
    desc.Width = sizeof(MaterialBuffer);
    hr = device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(materialBuffer_.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        std::cerr << "Failed to create material buffer." << std::endl;
        return false;
    }

    return true;
}

// 更新变换矩阵常量缓冲区
void DX12Renderer::UpdateConstantBuffer(const dx::XMMATRIX& worldMatrix, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    // 计算世界-视图-投影矩阵
    dx::XMMATRIX worldViewProj = worldMatrix * viewMatrix * projectionMatrix;

    // 准备常量缓冲区数据
    ConstantBuffer data;
    dx::XMStoreFloat4x4(&data.worldViewProj, dx::XMMatrixTranspose(worldViewProj));
    dx::XMStoreFloat4x4(&data.world, dx::XMMatrixTranspose(worldMatrix));

    // 映射并更新缓冲区
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    constantBuffer_->Map(0, &readRange, &mappedData);
    memcpy(mappedData, &data, sizeof(ConstantBuffer));
    constantBuffer_->Unmap(0, nullptr);

    // 设置根参数0（变换矩阵常量缓冲区）
    commandList_->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());
}

// 更新材质和光照常量缓冲区
void DX12Renderer::UpdateMaterialBuffer(const dx::XMFLOAT4& diffuseColor)
{
    // 准备材质缓冲区数据
    MaterialBuffer data;
    data.diffuseColor = diffuseColor;
    // 将XMFLOAT4转换为XMFLOAT3，只使用前三个分量
    data.lightPos.x = lightPosition_.x;
    data.lightPos.y = lightPosition_.y;
    data.lightPos.z = lightPosition_.z;
    data.padding1 = 0.0f;
    // 将XMFLOAT4转换为XMFLOAT3，只使用前三个分量
    data.lightColor.x = lightColor_.x;
    data.lightColor.y = lightColor_.y;
    data.lightColor.z = lightColor_.z;
    data.padding2 = 0.0f;

    // 映射并更新缓冲区
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    materialBuffer_->Map(0, &readRange, &mappedData);
    memcpy(mappedData, &data, sizeof(MaterialBuffer));
    materialBuffer_->Unmap(0, nullptr);

    // 设置根参数1（材质和光照常量缓冲区）
    commandList_->SetGraphicsRootConstantBufferView(1, materialBuffer_->GetGPUVirtualAddress());
}

// 渲染一帧
void DX12Renderer::Render(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    // 获取当前后台缓冲区
    currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += currentBackBufferIndex_ * rtvDescriptorSize_;

    // 重置当前帧的命令分配器
    HRESULT hr = commandAllocators_[currentFrameIndex_]->Reset();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command allocator.");
    }

    // 重置命令列表并关联当前帧的命令分配器
    hr = commandList_->Reset(commandAllocators_[currentFrameIndex_].Get(), nullptr);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command list.");
    }

    // 设置管道状态
    commandList_->SetGraphicsRootSignature(rootSignature_.Get());

    // 资源转换：设置渲染目标为渲染状态
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers_[currentBackBufferIndex_].Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList_->ResourceBarrier(1, &barrier);

    // 清除渲染目标和深度缓冲区 - 浅灰色背景
    const float clearColor[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(
        dsvHeap_->GetCPUDescriptorHandleForHeapStart(),
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr
    );

    // 设置渲染目标和深度/模板视图
    commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap_->GetCPUDescriptorHandleForHeapStart());

    // 设置视口和裁剪矩形
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList_->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(width_);
    scissorRect.bottom = static_cast<LONG>(height_);
    commandList_->RSSetScissorRects(1, &scissorRect);

    // 渲染布料
    if (clothVertexBuffer_ && clothIndexBuffer_ && clothVertexCount_ > 0 && clothIndexCount_ > 0)
    {
        // 设置布料管道状态
        commandList_->SetPipelineState(clothPipelineState_.Get());

        // 更新布料的变换矩阵和材质
        dx::XMMATRIX worldMatrix = dx::XMMatrixIdentity();
        UpdateConstantBuffer(worldMatrix, viewMatrix, projectionMatrix);
        UpdateMaterialBuffer(dx::XMFLOAT4(0.0f, 0.8f, 1.0f, 1.0f)); // 亮蓝色布料，更明显

        // 设置顶点和索引缓冲区
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = clothVertexBuffer_->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(dx::XMFLOAT3) * 2;  // 位置 + 法线
        vertexBufferView.SizeInBytes = clothVertexCount_ * sizeof(dx::XMFLOAT3) * 2;
        commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);

        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        indexBufferView.BufferLocation = clothIndexBuffer_->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBufferView.SizeInBytes = clothIndexCount_ * sizeof(uint32_t);
        commandList_->IASetIndexBuffer(&indexBufferView);

        // 设置图元拓扑
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 绘制布料
        commandList_->DrawIndexedInstanced(clothIndexCount_, 1, 0, 0, 0);
    }

    // 渲染球体
    if (sphereVertexBuffer_ && sphereIndexBuffer_ && sphereVertexCount_ > 0 && sphereIndexCount_ > 0)
    {
        // 设置球体管道状态
        commandList_->SetPipelineState(spherePipelineState_.Get());

        // 更新球体的变换矩阵和材质
        dx::XMMATRIX worldMatrix = dx::XMMatrixTranslation(0.0f, 5.0f, 0.0f);
        UpdateConstantBuffer(worldMatrix, viewMatrix, projectionMatrix);
        UpdateMaterialBuffer(dx::XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f)); // 红色球体

        // 设置顶点和索引缓冲区
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = sphereVertexBuffer_->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(dx::XMFLOAT3) * 2;  // 位置 + 法线
        vertexBufferView.SizeInBytes = sphereVertexCount_ * sizeof(dx::XMFLOAT3) * 2;
        commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);

        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        indexBufferView.BufferLocation = sphereIndexBuffer_->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBufferView.SizeInBytes = sphereIndexCount_ * sizeof(uint32_t);
        commandList_->IASetIndexBuffer(&indexBufferView);

        // 设置图元拓扑
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 绘制球体
        commandList_->DrawIndexedInstanced(sphereIndexCount_, 1, 0, 0, 0);
    }

    // 资源转换：设置渲染目标为呈现状态
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &barrier);

    // 关闭命令列表
    commandList_->Close();

    // 执行命令列表
    ID3D12CommandList* ppCommandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 呈现
    HRESULT presentHr = swapChain_->Present(1, 0);
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
    
    // 调试输出第一个顶点的位置和法线
    if (!positions.empty() && !normals.empty()) {
        std::cout << "First vertex: position = (" << positions[0].x << ", " << positions[0].y << ", " << positions[0].z << "), normal = (" << normals[0].x << ", " << normals[0].y << ", " << normals[0].z << ")" << std::endl;
    }

    if (positions.empty() || normals.empty() || indices.empty())
    {
        std::cout << "Warning: Empty vertex or index data received" << std::endl;
        return;
    }

    clothVertexCount_ = static_cast<uint32_t>(positions.size());
    clothIndexCount_ = static_cast<uint32_t>(indices.size());
    std::cout << "Setting cloth vertex count: " << clothVertexCount_ << ", index count: " << clothIndexCount_ << std::endl;

    // 创建顶点缓冲区
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    clothVertexBuffer_ = CreateBuffer(
        clothVertexCount_ * sizeof(dx::XMFLOAT3) * 2,  // 位置 + 法线
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        D3D12_RESOURCE_STATE_COMMON
    );

    // 创建索引缓冲区
    clothIndexBuffer_ = CreateBuffer(
        clothIndexCount_ * sizeof(uint32_t),
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        D3D12_RESOURCE_STATE_COMMON
    );

    // 准备顶点数据
    std::cout << "Preparing vertex data: " << clothVertexCount_ * sizeof(dx::XMFLOAT3) * 2 << " bytes" << std::endl;
    std::vector<BYTE> vertexData(clothVertexCount_ * sizeof(dx::XMFLOAT3) * 2);
    for (uint32_t i = 0; i < clothVertexCount_; ++i)
    {
        size_t positionOffset = i * sizeof(dx::XMFLOAT3) * 2;
        size_t normalOffset = positionOffset + sizeof(dx::XMFLOAT3);

        memcpy(&vertexData[positionOffset], &positions[i], sizeof(dx::XMFLOAT3));
        memcpy(&vertexData[normalOffset], &normals[i], sizeof(dx::XMFLOAT3));
    }

    // 上传顶点和索引数据
    std::cout << "Uploading vertex data to GPU" << std::endl;
    UploadBufferData(clothVertexBuffer_, vertexData);
    std::cout << "Uploading index data to GPU: " << clothIndexCount_ * sizeof(uint32_t) << " bytes" << std::endl;
    UploadBufferData(clothIndexBuffer_, indices);
    std::cout << "SetClothVertices completed successfully" << std::endl;
}

// 设置球体顶点数据
void DX12Renderer::SetSphereVertices(const std::vector<dx::XMFLOAT3>& positions, const std::vector<dx::XMFLOAT3>& normals,
                                   const std::vector<uint32_t>& indices)
{
    if (positions.empty() || normals.empty() || indices.empty())
    {
        return;
    }

    sphereVertexCount_ = static_cast<uint32_t>(positions.size());
    sphereIndexCount_ = static_cast<uint32_t>(indices.size());

    // 创建顶点缓冲区
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    sphereVertexBuffer_ = CreateBuffer(
        sphereVertexCount_ * sizeof(dx::XMFLOAT3) * 2,  // 位置 + 法线
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        D3D12_RESOURCE_STATE_COMMON
    );

    // 创建索引缓冲区
    sphereIndexBuffer_ = CreateBuffer(
        sphereIndexCount_ * sizeof(uint32_t),
        D3D12_RESOURCE_FLAG_NONE,
        heapProps,
        D3D12_RESOURCE_STATE_COMMON
    );

    // 准备顶点数据
    std::vector<BYTE> vertexData(sphereVertexCount_ * sizeof(dx::XMFLOAT3) * 2);
    for (uint32_t i = 0; i < sphereVertexCount_; ++i)
    {
        size_t positionOffset = i * sizeof(dx::XMFLOAT3) * 2;
        size_t normalOffset = positionOffset + sizeof(dx::XMFLOAT3);

        memcpy(&vertexData[positionOffset], &positions[i], sizeof(dx::XMFLOAT3));
        memcpy(&vertexData[normalOffset], &normals[i], sizeof(dx::XMFLOAT3));
    }

    // 上传顶点和索引数据
    UploadBufferData(sphereVertexBuffer_, vertexData);
    UploadBufferData(sphereIndexBuffer_, indices);
}



// 等待前一帧完成
void DX12Renderer::WaitForPreviousFrame()
{
    // 推进围栏值
    uint64_t currentFenceValue = ++fenceValue_;

    // 向命令队列添加围栏
    HRESULT hr = commandQueue_->Signal(fence_.Get(), currentFenceValue);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to signal fence.");
    }

    // 如果围栏值尚未完成，则等待
    if (fence_->GetCompletedValue() < currentFenceValue)
    {
        hr = fence_->SetEventOnCompletion(currentFenceValue, fenceEvent_);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to set event on fence completion.");
        }

        // 等待事件
        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    // 切换到下一帧的命令分配器
    currentFrameIndex_ = (currentFrameIndex_ + 1) % 2;
}

// 调整窗口大小
void DX12Renderer::Resize(uint32_t width, uint32_t height)
{
    // 等待所有命令完成
    WaitForPreviousFrame();

    // 保存新的窗口尺寸
    width_ = width;
    height_ = height;

    // 释放旧的渲染目标视图和深度/模板视图
    backBuffers_.clear();
    depthStencilBuffer_.Reset();

    // 调整交换链大小
    HRESULT hr = swapChain_->ResizeBuffers(
        backBufferCount_,
        width_,
        height_,
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
    if (fenceEvent_)
    {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }

    // 释放资源（智能指针会自动处理）
}

// 渲染整个场景
void DX12Renderer::RenderScene(const Scene& scene, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    // 获取当前后台缓冲区
    currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += currentBackBufferIndex_ * rtvDescriptorSize_;

    // 重置当前帧的命令分配器
    HRESULT hr = commandAllocators_[currentFrameIndex_]->Reset();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command allocator.");
    }

    // 重置命令列表并关联当前帧的命令分配器
    hr = commandList_->Reset(commandAllocators_[currentFrameIndex_].Get(), nullptr);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to reset command list.");
    }

    // 设置管道状态
    commandList_->SetGraphicsRootSignature(rootSignature_.Get());

    // 资源转换：设置渲染目标为渲染状态
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers_[currentBackBufferIndex_].Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList_->ResourceBarrier(1, &barrier);

    // 清除渲染目标和深度缓冲区
    const auto& bgColor = scene.getBackgroundColor();
    const float clearColor[] = { bgColor.x, bgColor.y, bgColor.z, bgColor.w };
    commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(
        dsvHeap_->GetCPUDescriptorHandleForHeapStart(),
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr
    );

    // 设置渲染目标和深度/模板视图
    commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap_->GetCPUDescriptorHandleForHeapStart());

    // 设置视口和裁剪矩形
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList_->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(width_);
    scissorRect.bottom = static_cast<LONG>(height_);
    commandList_->RSSetScissorRects(1, &scissorRect);

    // 保存光源位置和颜色到成员变量
    lightPosition_ = scene.getLightPosition();
    lightColor_ = scene.getLightColor();

    // 保存需要渲染的所有缓冲区引用
    std::vector<wrl::ComPtr<ID3D12Resource>> tempBuffers;
    
    // 遍历并渲染所有Primitive对象
    for (const auto& primitive : scene.getPrimitives())
    {
        // 确保Primitive有有效的顶点和索引数据
        const auto& positions = primitive->getPositions();
        const auto& normals = primitive->getNormals();
        const auto& indices = primitive->getIndices();

        if (positions.empty() || normals.empty() || indices.empty())
        {
            continue;
        }

        // 准备顶点数据（位置+法线）
        std::vector<float> vertexData;
        for (size_t i = 0; i < positions.size(); ++i)
        {
            // 添加位置
            vertexData.push_back(positions[i].x);
            vertexData.push_back(positions[i].y);
            vertexData.push_back(positions[i].z);
            // 添加法线
            vertexData.push_back(normals[i].x);
            vertexData.push_back(normals[i].y);
            vertexData.push_back(normals[i].z);
        }

        // 创建上传堆（用于CPU到GPU的数据传输）
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;

        // 创建上传顶点缓冲区
        wrl::ComPtr<ID3D12Resource> vertexUploadBuffer = CreateBuffer(
            vertexData.size() * sizeof(float),
            D3D12_RESOURCE_FLAG_NONE,
            uploadHeapProps,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
        tempBuffers.push_back(vertexUploadBuffer);

        // 创建上传索引缓冲区
        wrl::ComPtr<ID3D12Resource> indexUploadBuffer = CreateBuffer(
            indices.size() * sizeof(uint32_t),
            D3D12_RESOURCE_FLAG_NONE,
            uploadHeapProps,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
        tempBuffers.push_back(indexUploadBuffer);

        // 复制数据到上传缓冲区
        void* vertexDataPtr = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        vertexUploadBuffer->Map(0, &readRange, &vertexDataPtr);
        memcpy(vertexDataPtr, vertexData.data(), vertexData.size() * sizeof(float));
        vertexUploadBuffer->Unmap(0, nullptr);

        void* indexDataPtr = nullptr;
        indexUploadBuffer->Map(0, &readRange, &indexDataPtr);
        memcpy(indexDataPtr, indices.data(), indices.size() * sizeof(uint32_t));
        indexUploadBuffer->Unmap(0, nullptr);

        // 创建默认堆（GPU访问的顶点缓冲区）
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeapProps.CreationNodeMask = 1;
        defaultHeapProps.VisibleNodeMask = 1;

        // 创建默认顶点缓冲区
        wrl::ComPtr<ID3D12Resource> vertexBuffer = CreateBuffer(
            vertexData.size() * sizeof(float),
            D3D12_RESOURCE_FLAG_NONE,
            defaultHeapProps,
            D3D12_RESOURCE_STATE_COPY_DEST
        );
        tempBuffers.push_back(vertexBuffer);

        // 创建默认索引缓冲区
        wrl::ComPtr<ID3D12Resource> indexBuffer = CreateBuffer(
            indices.size() * sizeof(uint32_t),
            D3D12_RESOURCE_FLAG_NONE,
            defaultHeapProps,
            D3D12_RESOURCE_STATE_COPY_DEST
        );
        tempBuffers.push_back(indexBuffer);

        // 从上传堆复制数据到默认堆
        commandList_->CopyResource(vertexBuffer.Get(), vertexUploadBuffer.Get());
        commandList_->CopyResource(indexBuffer.Get(), indexUploadBuffer.Get());

        // 转换缓冲区状态
        barrier.Transition.pResource = vertexBuffer.Get();
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        commandList_->ResourceBarrier(1, &barrier);

        barrier.Transition.pResource = indexBuffer.Get();
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        commandList_->ResourceBarrier(1, &barrier);

        // 设置管道状态
        commandList_->SetPipelineState(clothPipelineState_.Get());

        // 更新常量缓冲区（使用Primitive的世界矩阵）
        UpdateConstantBuffer(primitive->getWorldMatrix(), viewMatrix, projectionMatrix);
        
        // 更新材质缓冲区（使用Primitive的颜色）
        UpdateMaterialBuffer(primitive->getDiffuseColor());

        // 设置顶点和索引缓冲区
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(float) * 6;  // 位置(3) + 法线(3)
        vertexBufferView.SizeInBytes = vertexData.size() * sizeof(float);
        commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);

        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBufferView.SizeInBytes = indices.size() * sizeof(uint32_t);
        commandList_->IASetIndexBuffer(&indexBufferView);

        // 设置图元拓扑
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 绘制图元
        commandList_->DrawIndexedInstanced(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    // 资源转换：设置渲染目标为呈现状态
    barrier.Transition.pResource = backBuffers_[currentBackBufferIndex_].Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &barrier);

    // 关闭命令列表
    commandList_->Close();

    // 执行命令列表
    ID3D12CommandList* ppCommandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 呈现
    HRESULT presentHr = swapChain_->Present(1, 0);
    if (FAILED(presentHr))
    {
        throw std::runtime_error("Failed to present swap chain.");
    }

    // 等待当前帧完成
    WaitForPreviousFrame();
}