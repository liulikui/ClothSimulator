#include "Scene.h"
#include "Primitive.h"
#include "Cloth.h"
#include "Sphere.h"
#include "DX12RALDevice.h"
#include "RALResource.h"
#include "TRefCountPtr.h"
#include <algorithm>
#include <iostream>
#include <string>

// 日志函数声明
extern void logDebug(const std::string& message);

struct SceneConstBuffer 
{
    dx::XMFLOAT4X4 View;            // 世界矩阵
    dx::XMFLOAT4X4 Proj;            // 投影矩阵
    dx::XMFLOAT4X4 ViewProj;        // 视图-投影矩阵
    dx::XMFLOAT3 lightPos;          // 光源位置
    float padding1;                 // 4字节对齐填充
    dx::XMFLOAT4 lightDiffuseColor; // 光源漫反射颜色
    dx::XMFLOAT4 lightSpecularColor; // 光源高光颜色
};

struct ObjectConstBuffer
{
    dx::XMFLOAT4X4 World;           // 世界矩阵
    dx::XMFLOAT3 diffuseColor;      // 漫反射颜色
    float padding1;                 // 4字节对齐填充
};

Scene::Scene()
    : m_device(nullptr)
{
    // 初始化场景
    // cameraConstBuffer将在渲染器中创建并传入
    logDebug("[DEBUG] Scene constructor called");
}

bool Scene::Initialize(IRALDevice* pDevice)
{
    if (!pDevice)
    {
        logDebug("[DEBUG] Scene::Initialize failed: device is null");
        return false;
    }

    m_device = pDevice;
    
    logDebug("[DEBUG] Scene::Initialize called");
    
    // 定义根参数
    std::vector<RALRootParameter> rootParameters(2);

    // 根参数0: 常量缓冲区视图（场景常量）
    InitAsConstantBufferView(rootParameters[0], 0, 0, RALShaderVisibility::All);
    // 根参数1: 常量缓冲区视图（对象常量）
    InitAsConstantBufferView(rootParameters[1], 1, 0, RALShaderVisibility::All);

    // 定义静态采样器
    RALStaticSampler staticSampler;
    InitStaticSampler(
        staticSampler,
        RALFilter::MinMagMipLinear,
        RALTextureAddressMode::Wrap,
        RALTextureAddressMode::Wrap,
        RALTextureAddressMode::Wrap,
        0.0f,                   // MipLODBias
        1,                      // MaxAnisotropy
        RALComparisonFunc::Always,
        RALStaticBorderColor::TransparentBlack,
        0.0f,                   // MinLOD
        3.402823466e+38f,       // MaxLOD
        0,                      // ShaderRegister
        0,                      // RegisterSpace
        RALShaderVisibility::Pixel
    );
    
    std::vector<RALStaticSampler> staticSamplers = { staticSampler };

    // 使用DX12Renderer的CreateRootSignature方法创建根签名
    IRALRootSignature* rootSignaturePtr = pDevice->CreateRootSignature(
        rootParameters,
        staticSamplers,
        RALRootSignatureFlags::AllowInputAssemblerInputLayout
    );
    
    if (!rootSignaturePtr)
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to create root signature");
        return false;
    }
    
    // 保存根签名到成员变量
    m_rootSignature = rootSignaturePtr;
    
    logDebug("[DEBUG] Scene::Initialize succeeded: root signature created and stored");
    
    // 定义着色器代码
    const char* vertexShaderCode = ""
        "struct VS_INPUT {\n"
        "   float3 pos : POSITION;\n"
        "   float3 normal : NORMAL;\n"
        "};\n"
        "struct VS_OUTPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float3 normal : NORMAL;\n"
        "   float3 worldPos : WORLD_POS;\n"
        "};\n"
        "cbuffer SceneBuffer : register(b0) {\n"
        "   float4x4 View;\n"
        "   float4x4 Proj;\n"
        "   float4x4 ViewProj;\n"
        "   float3 lightPos;\n"
        "   float4 lightDiffuseColor;\n"
        "   float4 lightSpecularColor;\n"
        "};\n"
        "cbuffer ObjectBuffer : register(b1) {\n"
        "   float4x4 World;\n"
        "   float4 diffuseColor;\n"
        "};\n"
        "VS_OUTPUT main(VS_INPUT input) {\n"
        "   VS_OUTPUT output;\n"
        "   float4x4 worldViewProj = mul(World, ViewProj);\n"
        "   output.pos = mul(float4(input.pos, 1.0f), worldViewProj);\n"
        "   output.worldPos = mul(float4(input.pos, 1.0f), World).xyz;\n"
        "   float4 normal = float4(input.normal, 0.0f);\n"
        "   output.normal = mul(input.normal, World).xyz;\n"
        "   // Ensure normal is normalized for correct lighting\n"
        "   output.normal = normalize(output.normal);\n"
        "   return output;\n"
        "}";

    const char* pixelShaderCode = ""
        "struct PS_INPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float3 normal : NORMAL;\n"
        "   float3 worldPos : WORLD_POS;\n"
        "};\n"
        "cbuffer SceneBuffer : register(b0) {\n"
        "   float4x4 View;\n"
        "   float4x4 Proj;\n"
        "   float4x4 ViewProj;\n"
        "   float3 lightPos;\n"
        "   float4 lightDiffuseColor;\n"
        "   float4 lightSpecularColor;\n"
        "};\n"
        "cbuffer ObjectBuffer : register(b1) {\n"
        "   float4x4 World;\n"
        "   float4 diffuseColor;\n"
        "};\n"
        "float4 main(PS_INPUT input) : SV_TARGET {\n"
        "   float3 normal = normalize(input.normal);\n"
        "   float3 lightDir = normalize(lightPos - input.worldPos);\n"
        "   float3 viewDir = normalize(float3(0, 0, 5) - input.worldPos);\n"
        "   float3 halfVec = normalize(lightDir + viewDir);\n"
        "   \n"
        "   float diffuse = max(dot(normal, lightDir), 0.0f);\n"
        "   float specular = pow(max(dot(normal, halfVec), 0.0f), 32.0f);\n"
        "   \n"
        "   float3 ambient = float3(0.2f, 0.2f, 0.2f);\n"
        "   float3 finalColor = ambient + diffuse * diffuseColor.rgb * lightDiffuseColor + specular * lightSpecularColor;\n"
        "   \n"
        "   return float4(finalColor, diffuseColor.a);\n"
        "}";

    // 编译着色器
    m_vertexShader = pDevice->CompileVertexShader(vertexShaderCode, "main");
    if (!m_vertexShader.Get())
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to compile vertex shader");
        return false;
    }

    m_pixelShader = pDevice->CompilePixelShader(pixelShaderCode, "main");
    if (!m_pixelShader.Get())
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to compile pixel shader");
        return false;
    }

    logDebug("[DEBUG] Scene::Initialize succeeded: shaders compiled and stored");
    
    // 创建图形管道状态
    RALGraphicsPipelineStateDesc pipelineDesc = {};
    
    // 配置输入布局
    std::vector<RALVertexAttribute> inputLayout;
    RALVertexAttribute posAttr;
    posAttr.semantic = RALVertexSemantic::Position;
    posAttr.format = RALVertexFormat::Float3;
    posAttr.bufferSlot = 0;
    posAttr.offset = 0;
    inputLayout.push_back(posAttr);
    
    RALVertexAttribute normalAttr;
    normalAttr.semantic = RALVertexSemantic::Normal;
    normalAttr.format = RALVertexFormat::Float3;
    normalAttr.bufferSlot = 0;
    normalAttr.offset = 12; // 3个float，每个4字节，共12字节
    inputLayout.push_back(normalAttr);
    
    pipelineDesc.inputLayout = &inputLayout;
    
    // 设置根签名
    pipelineDesc.rootSignature = m_rootSignature.Get();
    
    // 设置着色器
    pipelineDesc.vertexShader = m_vertexShader.Get();
    pipelineDesc.pixelShader = m_pixelShader.Get();
    
    // 设置图元拓扑类型
    pipelineDesc.primitiveTopologyType = RALPrimitiveTopologyType::TriangleList;
    
    // 配置光栅化状态
    pipelineDesc.rasterizerState.cullMode = RALCullMode::None; // 关闭剔除，启用双面渲染
    pipelineDesc.rasterizerState.fillMode = RALFillMode::Solid;
    pipelineDesc.rasterizerState.frontCounterClockwise = false;
    pipelineDesc.rasterizerState.depthClipEnable = true;
    pipelineDesc.rasterizerState.multisampleEnable = false;
    
    // 配置混合状态
    pipelineDesc.blendState.alphaToCoverageEnable = false;
    pipelineDesc.blendState.independentBlendEnable = false;
    
    // 添加默认渲染目标混合状态
    RALRenderTargetBlendState defaultBlendState;
    defaultBlendState.blendEnable = false;
    defaultBlendState.logicOpEnable = false;
    defaultBlendState.srcBlend = RALBlendFactor::One;
    defaultBlendState.destBlend = RALBlendFactor::Zero;
    defaultBlendState.blendOp = RALBlendOp::Add;
    defaultBlendState.srcBlendAlpha = RALBlendFactor::One;
    defaultBlendState.destBlendAlpha = RALBlendFactor::Zero;
    defaultBlendState.blendOpAlpha = RALBlendOp::Add;
    defaultBlendState.logicOp = RALLogicOp::Noop;
    defaultBlendState.colorWriteMask = 0xF; // RGBA
    pipelineDesc.renderTargetBlendStates.push_back(defaultBlendState);
    
    // 配置深度模板状态
    pipelineDesc.depthStencilState.depthEnable = true;
    pipelineDesc.depthStencilState.depthWriteMask = true;
    pipelineDesc.depthStencilState.depthFunc = RALCompareOp::Less;
    pipelineDesc.depthStencilState.stencilEnable = false;
    
    // 配置渲染目标格式
    pipelineDesc.numRenderTargets = 1;
    pipelineDesc.renderTargetFormats[0] = DataFormat::R8G8B8A8_UNorm;
    pipelineDesc.depthStencilFormat = DataFormat::D32_Float;
    
    // 配置多重采样
    pipelineDesc.sampleDesc.Count = 1;
    pipelineDesc.sampleDesc.Quality = 0;
    pipelineDesc.sampleMask = UINT32_MAX;
    
    // 创建图形管道状态
    IRALGraphicsPipelineState* pipelineStatePtr = pDevice->CreateGraphicsPipelineState(pipelineDesc);
    if (!pipelineStatePtr)
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to create graphics pipeline state");
        return false;
    }
    
    // 保存图形管道状态到成员变量
    m_pipelineState = pipelineStatePtr;
    
    logDebug("[DEBUG] Scene::Initialize succeeded: graphics pipeline state created and stored");
    
    m_constBuffer = pDevice->CreateConstBuffer(sizeof(SceneConstBuffer));
    if (!m_constBuffer.Get())
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to create const buffer");
        return false;
    }

    return true;
}

Scene::~Scene()
{
    // 清空场景中的所有对象
    Clear();
}

void Scene::Update(float deltaTime)
{
    UpdatePrimitiveRequests();

    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();

    // 更新场景中所有可见对象的状态
    for (auto& primitiveInfo : m_primitives) 
    {
        if (primitiveInfo.primitive && primitiveInfo.visible)
        {
            primitiveInfo.primitive->Update(commandList, deltaTime);
        }
     }
}

void Scene::Render(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();

    // 更新场景常量缓冲区
    UpdateSceneConstBuffer(commandList, viewMatrix, projectionMatrix);
    // 设置管道签名
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    // 设置管线状态
    commandList->SetPipelineState(m_pipelineState.Get());
    // 设置根参数0（变换矩阵常量缓冲区）
    commandList->SetGraphicsRootConstantBuffer(0, m_constBuffer.Get());
    // 设置图元拓扑
    commandList->SetPrimitiveTopology(RALPrimitiveTopologyType::TriangleList);

    // 渲染每个可见的Primitive对象
    for (size_t i = 0; i < m_primitives.size(); ++i)
    {
        auto& primitiveInfo = m_primitives[i];

        if (primitiveInfo.primitive && primitiveInfo.visible)
        {
            // 获取对象的世界矩阵、材质颜色、顶点数据和索引数据
            const dx::XMMATRIX& worldMatrix = primitiveInfo.worldMatrix;
			IRALVertexBuffer* vertexBuffer = primitiveInfo.vertexBuffer.Get();
			IRALIndexBuffer* indexBuffer = primitiveInfo.indexBuffer.Get();

            if (vertexBuffer == nullptr || indexBuffer == nullptr)
            {
                continue;
            }

            PrimitiveMesh mesh;
            mesh.vertexBuffer = primitiveInfo.vertexBuffer.Get();
            mesh.indexBuffer = primitiveInfo.indexBuffer.Get();

            // 更新Mesh
            primitiveInfo.primitive->OnUpdateMesh(m_device, mesh);

            // 更新Primitive常量缓冲区
            UpdatePrimitiveConstBuffer(commandList, &primitiveInfo);

            commandList->SetVertexBuffers(0, 1, &vertexBuffer);
            commandList->SetIndexBuffer(indexBuffer);

            // 设置根参数1（对象常量缓冲区）
            commandList->SetGraphicsRootConstantBuffer(1, primitiveInfo.constBuffer.Get());
            // 绘制对象
			commandList->DrawIndexed(indexBuffer->GetIndexCount(), 1, 0, 0, 0);
        }
    }
}

bool Scene::AddPrimitive(Primitive* primitive)
{
    if (!primitive)
    {
        return false;
    }

    // 检查对象是否已经在场景中
    for (std::vector<PrimitiveInfo>::iterator iter = m_primitives.begin(); iter != m_primitives.end(); ++iter)
    {
        if (iter->primitive == primitive)
        {
            return false;
        }
    }

    AddPrimitiveRequest request;
    request.primitive = primitive;

    m_addPrimitiveRequests.push_back(request);
    
    return true;
}

bool Scene::RemovePrimitive(Primitive* primitive) {
    if (!primitive)
    {
        return false;
    }

    /// 检查对象是否已经在场景中
    for (std::vector<PrimitiveInfo>::iterator iter = m_primitives.begin(); iter != m_primitives.end(); ++iter)
    {
        if (iter->primitive == primitive)
        {
            m_primitives.erase(iter);
            return true;
        }
    }

    return false;
}

void Scene::Clear()
{
    // 清空所有对象
    m_primitives.clear();
}

void Scene::UpdatePrimitiveRequests()
{
    for (std::vector<AddPrimitiveRequest>::iterator iter = m_addPrimitiveRequests.begin(); iter != m_addPrimitiveRequests.end(); ++iter)
    {
        Primitive* primitive = iter->primitive;
        PrimitiveInfo primitiveInfo;
        primitiveInfo.primitive = primitive;
        primitiveInfo.worldMatrix = primitive->GetWorldMatrix();
        primitiveInfo.visible = primitive->IsVisible();

        PrimitiveMesh mesh;
        primitive->OnSetupMesh(m_device, mesh);
        
        primitiveInfo.vertexBuffer = mesh.vertexBuffer;
        primitiveInfo.indexBuffer = mesh.indexBuffer;
        
        primitiveInfo.diffuseColor = primitive->GetDiffuseColor();
        primitiveInfo.constBuffer = m_device->CreateConstBuffer(sizeof(ObjectConstBuffer));

        // 添加对象到场景中
        m_primitives.push_back(primitiveInfo);
    }

    m_addPrimitiveRequests.clear();
}

void Scene::UpdateSceneConstBuffer(IRALGraphicsCommandList* commandList, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    // 计算视图-投影矩阵
    dx::XMMATRIX viewProjMatrix = viewMatrix * projectionMatrix;

    SceneConstBuffer data;
    dx::XMStoreFloat4x4(&data.View, dx::XMMatrixTranspose(viewMatrix));
    dx::XMStoreFloat4x4(&data.Proj, dx::XMMatrixTranspose(projectionMatrix));
    dx::XMStoreFloat4x4(&data.ViewProj, dx::XMMatrixTranspose(viewProjMatrix));
    data.lightPos = m_lightPosition;
    data.lightDiffuseColor = m_lightDiffuseColor;
    data.lightSpecularColor = m_lightSpecularColor;
    data.padding1 = 0.0f;

    // 映射并更新缓冲区
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    m_constBuffer.Get()->Map(&mappedData);
    memcpy(mappedData, &data, sizeof(SceneConstBuffer));
    m_constBuffer.Get()->Unmap();
}

void Scene::UpdatePrimitiveConstBuffer(IRALGraphicsCommandList* commandList, PrimitiveInfo* primitiveInfo)
{
    ObjectConstBuffer data;
    dx::XMStoreFloat4x4(&data.World, dx::XMMatrixTranspose(primitiveInfo->worldMatrix));
    data.diffuseColor = primitiveInfo->diffuseColor;

    // 映射并更新缓冲区
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    primitiveInfo->constBuffer->Map(&mappedData);
    memcpy(mappedData, &data, sizeof(SceneConstBuffer));
    primitiveInfo->constBuffer->Unmap();
}