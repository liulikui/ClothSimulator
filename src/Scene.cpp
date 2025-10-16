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
    
    m_sceneConstBuffer = pDevice->CreateConstBuffer(sizeof(SceneConstBuffer));
    if (!m_sceneConstBuffer.Get())
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to create scene const buffer");
        return false;
    }

    // 初始化延迟着色相关资源
    if (!InitializeDeferredRendering())
    {
        logDebug("[DEBUG] Scene::Initialize failed: failed to initialize deferred rendering");
        return false;
    }

    return true;
}

Scene::~Scene()
{
    // 清空场景中的所有对象
    Clear();
    
    // 清理延迟着色相关资源
    CleanupDeferredRendering();
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
    if (!m_device)
    {
        logDebug("[DEBUG] Scene::Render failed: device is null");
        return;
    }

    // 执行延迟着色的两个主要阶段
    // 1. 几何阶段：渲染场景到GBuffer
    ExecuteGeometryPass(viewMatrix, projectionMatrix);
    
    // 2. 光照阶段：使用GBuffer中的信息进行光照计算
    ExecuteLightingPass();
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
    m_sceneConstBuffer.Get()->Map(&mappedData);
    memcpy(mappedData, &data, sizeof(SceneConstBuffer));
    m_sceneConstBuffer.Get()->Unmap();
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
    memcpy(mappedData, &data, sizeof(ObjectConstBuffer));
    primitiveInfo->constBuffer->Unmap();
}

// 延迟着色光照阶段常量缓冲区
struct LightPassConstBuffer
{
    dx::XMFLOAT3 lightPos;          // 光源位置
    float padding1;                 // 4字节对齐填充
    dx::XMFLOAT4 lightDiffuseColor; // 光源漫反射颜色
    dx::XMFLOAT4 lightSpecularColor; // 光源高光颜色
};

// 初始化延迟着色相关资源
bool Scene::InitializeDeferredRendering()
{
    if (!m_device)
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: device is null");
        return false;
    }

    logDebug("[DEBUG] Scene::InitializeDeferredRendering called");

    uint32_t width = m_device->GetWidth();
    uint32_t height = m_device->GetHeight();

    // 创建GBuffer纹理
    // GBufferA: 世界空间法线 (RGBA16F用于精度)
    m_gbufferA = m_device->CreateRenderTarget(width, height, RALDataFormat::R16G16B16A16_Float);
    if (!m_gbufferA.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create GBufferA");
        return false;
    }

    // GBufferB: Metallic (R), Specular (G), Roughness (B) (RGB8_UNorm)
    m_gbufferB = m_device->CreateRenderTarget(width, height, RALDataFormat::R8G8B8A8_UNorm);
    if (!m_gbufferB.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create GBufferB");
        return false;
    }

    // GBufferC: BaseColor RGB (RGB8_UNorm)
    m_gbufferC = m_device->CreateRenderTarget(width, height, RALDataFormat::R8G8B8A8_UNorm);
    if (!m_gbufferC.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create GBufferC");
        return false;
    }

    // 创建深度/模板缓冲区
    m_gbufferDepthStencil = m_device->CreateDepthStencil(width, height, RALDataFormat::D32_Float);
    if (!m_gbufferDepthStencil.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create depth stencil");
        return false;
    }

    // 创建光照阶段常量缓冲区
    m_lightPassConstBuffer = m_device->CreateConstBuffer(sizeof(LightPassConstBuffer));
    if (!m_lightPassConstBuffer.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create light pass const buffer");
        return false;
    }

    // 创建全屏四边形
    CreateFullscreenQuad();

    // 创建几何阶段根签名
    std::vector<RALRootParameter> gbufferRootParameters(2);
    InitAsConstantBufferView(gbufferRootParameters[0], 0, 0, RALShaderVisibility::All);
    InitAsConstantBufferView(gbufferRootParameters[1], 1, 0, RALShaderVisibility::All);

    // 定义GBuffer几何阶段根签名
    m_gbufferRootSignature = m_device->CreateRootSignature(
        gbufferRootParameters,
        {}, // 暂时不需要静态采样器
        RALRootSignatureFlags::AllowInputAssemblerInputLayout
    );

    if (!m_gbufferRootSignature.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create GBuffer root signature");
        return false;
    }

    // 定义几何阶段着色器代码
    const char* gbufferVSCode = 
        "struct VS_INPUT {\n"
        "   float3 pos : POSITION;\n"
        "   float3 normal : NORMAL;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "struct VS_OUTPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float3 normal : NORMAL;\n"
        "   float3 worldPos : WORLD_POS;\n"
        "   float2 uv : TEXCOORD;\n"
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
        "   // 将法线转换到世界空间并归一化\n"
        "   float4 normal = mul(float4(input.normal, 0.0f), World);\n"
        "   output.normal = normalize(normal.xyz);\n"
        "   output.uv = input.uv;\n"
        "   return output;\n"
        "}";

    const char* gbufferPSCode = 
        "struct PS_INPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float3 normal : NORMAL;\n"
        "   float3 worldPos : WORLD_POS;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "cbuffer ObjectBuffer : register(b1) {\n"
        "   float4x4 World;\n"
        "   float4 diffuseColor;\n"
        "};\n"
        "// 输出到GBuffer\n"
        "struct PS_OUTPUT {\n"
        "   float4 gbufferA : SV_TARGET0; // 世界空间法线\n"
        "   float4 gbufferB : SV_TARGET1; // Metallic, Specular, Roughness, 1.0\n"
        "   float4 gbufferC : SV_TARGET2; // BaseColor RGB\n"
        "};\n"
        "PS_OUTPUT main(PS_INPUT input) {\n"
        "   PS_OUTPUT output;\n"
        "   \n"
        "   // 输出世界空间法线到GBufferA\n"
        "   output.gbufferA.xyz = input.normal * 0.5f + 0.5f; // 将[-1,1]映射到[0,1]\n"
        "   output.gbufferA.w = 1.0f;\n"
        "   \n"
        "   // 输出材质属性到GBufferB\n"
        "   output.gbufferB.r = 0.5f; // 金属度 (暂时固定值)\n"
        "   output.gbufferB.g = 1.0f; // 高光强度 (暂时固定值)\n"
        "   output.gbufferB.b = 0.5f; // 粗糙度 (暂时固定值)\n"
        "   output.gbufferB.a = 1.0f;\n"
        "   \n"
        "   // 输出基础颜色到GBufferC\n"
        "   output.gbufferC.rgb = diffuseColor.rgb;\n"
        "   output.gbufferC.a = diffuseColor.a;\n"
        "   \n"
        "   return output;\n"
        "}";

    // 编译GBuffer着色器
    m_gbufferVertexShader = m_device->CompileVertexShader(gbufferVSCode, "main");
    if (!m_gbufferVertexShader.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile GBuffer vertex shader");
        return false;
    }

    m_gbufferPixelShader = m_device->CompilePixelShader(gbufferPSCode, "main");
    if (!m_gbufferPixelShader.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile GBuffer pixel shader");
        return false;
    }

    // 创建GBuffer几何阶段管道状态
    RALGraphicsPipelineStateDesc gbufferPipelineDesc = {};

    // 配置输入布局
    std::vector<RALVertexAttribute> gbufferInputLayout;
    RALVertexAttribute posAttr;
    posAttr.semantic = RALVertexSemantic::Position;
    posAttr.format = RALVertexFormat::Float3;
    posAttr.bufferSlot = 0;
    posAttr.offset = 0;
    gbufferInputLayout.push_back(posAttr);

    RALVertexAttribute normalAttr;
    normalAttr.semantic = RALVertexSemantic::Normal;
    normalAttr.format = RALVertexFormat::Float3;
    normalAttr.bufferSlot = 0;
    normalAttr.offset = 12; // 3个float，每个4字节，共12字节
    gbufferInputLayout.push_back(normalAttr);

    RALVertexAttribute uvAttr;
    uvAttr.semantic = RALVertexSemantic::TexCoord0;
    uvAttr.format = RALVertexFormat::Float2;
    uvAttr.bufferSlot = 0;
    uvAttr.offset = 24; // 6个float，每个4字节，共24字节
    gbufferInputLayout.push_back(uvAttr);

    gbufferPipelineDesc.inputLayout = &gbufferInputLayout;
    gbufferPipelineDesc.rootSignature = m_gbufferRootSignature.Get();
    gbufferPipelineDesc.vertexShader = m_gbufferVertexShader.Get();
    gbufferPipelineDesc.pixelShader = m_gbufferPixelShader.Get();
    gbufferPipelineDesc.primitiveTopologyType = RALPrimitiveTopologyType::TriangleList;

    // 配置光栅化状态
    gbufferPipelineDesc.rasterizerState.cullMode = RALCullMode::None;
    gbufferPipelineDesc.rasterizerState.fillMode = RALFillMode::Solid;
    gbufferPipelineDesc.rasterizerState.depthClipEnable = true;

    // 配置混合状态
    gbufferPipelineDesc.blendState.alphaToCoverageEnable = false;
    gbufferPipelineDesc.blendState.independentBlendEnable = false;
    
    // 添加默认渲染目标混合状态
    RALRenderTargetBlendState gbufferBlendState;
    gbufferBlendState.blendEnable = false;
    gbufferBlendState.logicOpEnable = false;
    gbufferBlendState.colorWriteMask = 0xF;
    for (int i = 0; i < 3; ++i)
    {
        gbufferPipelineDesc.renderTargetBlendStates.push_back(gbufferBlendState);
    }

    // 配置深度模板状态
    gbufferPipelineDesc.depthStencilState.depthEnable = true;
    gbufferPipelineDesc.depthStencilState.depthWriteMask = true;
    gbufferPipelineDesc.depthStencilState.depthFunc = RALCompareOp::Less;

    // 配置渲染目标格式
    gbufferPipelineDesc.numRenderTargets = 3;
    gbufferPipelineDesc.renderTargetFormats[0] = RALDataFormat::R16G16B16A16_Float; // GBufferA
    gbufferPipelineDesc.renderTargetFormats[1] = RALDataFormat::R8G8B8A8_UNorm;   // GBufferB
    gbufferPipelineDesc.renderTargetFormats[2] = RALDataFormat::R8G8B8A8_UNorm;   // GBufferC
    gbufferPipelineDesc.depthStencilFormat = RALDataFormat::D32_Float;

    // 创建几何阶段管道状态
    m_gbufferPipelineState = m_device->CreateGraphicsPipelineState(gbufferPipelineDesc);
    if (!m_gbufferPipelineState.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create GBuffer pipeline state");
        return false;
    }

    // 创建光照阶段根签名
    std::vector<RALRootParameter> lightRootParameters(2);
    InitAsConstantBufferView(lightRootParameters[0], 0, 0, RALShaderVisibility::Pixel);
    
    // 使用描述符表而不是根描述符SRV，因为根描述符SRV只能用于Raw或结构化缓冲区
    std::vector<RALRootDescriptorTableRange> ranges;
    RALRootDescriptorTableRange range;
    range.Type = RALDescriptorRangeType::SRV;
    range.NumDescriptors = 3; // 3个GBuffer纹理
    range.BaseShaderRegister = 0;
    range.RegisterSpace = 0;
    ranges.push_back(range);
    InitAsDescriptorTable(lightRootParameters[1], ranges, RALShaderVisibility::Pixel);

    // 定义静态采样器
    RALStaticSampler lightSampler;
    InitStaticSampler(
        lightSampler,
        RALFilter::MinMagMipLinear,
        RALTextureAddressMode::Clamp,
        RALTextureAddressMode::Clamp,
        RALTextureAddressMode::Clamp,
        0.0f,
        1,
        RALComparisonFunc::Always,
        RALStaticBorderColor::TransparentBlack,
        0.0f,
        3.402823466e+38f,
        0,
        0,
        RALShaderVisibility::Pixel
    );

    std::vector<RALStaticSampler> lightSamplers = { lightSampler };

    // 创建光照阶段根签名
    m_lightRootSignature = m_device->CreateRootSignature(
        lightRootParameters,
        lightSamplers,
        RALRootSignatureFlags::AllowInputAssemblerInputLayout
    );
    if (!m_lightRootSignature.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create light pass root signature");
        return false;
    }

    // 定义光照阶段着色器代码
    const char* lightVSCode = 
        "struct VS_INPUT {\n"
        "   float4 pos : POSITION;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "struct VS_OUTPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "VS_OUTPUT main(VS_INPUT input) {\n"
        "   VS_OUTPUT output;\n"
        "   output.pos = input.pos;\n"
        "   output.uv = input.uv;\n"
        "   return output;\n"
        "}";

    const char* lightPSCode = 
        "struct PS_INPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "cbuffer LightBuffer : register(b0) {\n"
        "   float3 lightPos;\n"
        "   float padding1;\n"
        "   float4 lightDiffuseColor;\n"
        "   float4 lightSpecularColor;\n"
        "};\n"
        "// 采样GBuffer纹理\n"
        "Texture2D<float4> gbufferA : register(t0);\n"
        "Texture2D<float4> gbufferB : register(t1);\n"
        "Texture2D<float4> gbufferC : register(t2);\n"
        "SamplerState samplerGBuffer : register(s0);\n"
        "float4 main(PS_INPUT input) : SV_TARGET {\n"
        "   // 从GBuffer中采样数据\n"
        "   float4 normalSample = gbufferA.Sample(samplerGBuffer, input.uv);\n"
        "   float4 materialSample = gbufferB.Sample(samplerGBuffer, input.uv);\n"
        "   float4 baseColorSample = gbufferC.Sample(samplerGBuffer, input.uv);\n"
        "   \n"
        "   // 将法线从[0,1]映射回[-1,1]\n"
        "   float3 normal = (normalSample.xyz * 2.0f) - 1.0f;\n"
        "   normal = normalize(normal);\n"
        "   \n"
        "   // 提取材质属性\n"
        "   float metallic = materialSample.r;\n"
        "   float specular = materialSample.g;\n"
        "   float roughness = materialSample.b;\n"
        "   \n"
        "   // 简化的光照计算\n"
        "   // 注意：这里假设摄像机在原点，世界空间位置需要额外的深度重建\n"
        "   float3 viewDir = normalize(float3(0, 0, 5) - float3(0, 0, 0));\n"
        "   float3 lightDir = normalize(lightPos - float3(0, 0, 0));\n"
        "   float3 halfVec = normalize(lightDir + viewDir);\n"
        "   \n"
        "   float diffuse = max(dot(normal, lightDir), 0.0f);\n"
        "   float specularTerm = pow(max(dot(normal, halfVec), 0.0f), 32.0f);\n"
        "   \n"
        "   float3 ambient = float3(0.2f, 0.2f, 0.2f);\n"
        "   float3 finalColor = ambient + diffuse * baseColorSample.rgb * lightDiffuseColor + specularTerm * specular * lightSpecularColor;\n"
        "   \n"
        "   return float4(finalColor, baseColorSample.a);\n"
        "}";

    // 编译光照阶段着色器
    m_lightVertexShader = m_device->CompileVertexShader(lightVSCode, "main");
    if (!m_lightVertexShader.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile light pass vertex shader");
        return false;
    }

    m_lightPixelShader = m_device->CompilePixelShader(lightPSCode, "main");
    if (!m_lightPixelShader.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile light pass pixel shader");
        return false;
    }

    // 创建光照阶段管道状态
    RALGraphicsPipelineStateDesc lightPipelineDesc = {};

    // 配置输入布局 (全屏四边形)
    std::vector<RALVertexAttribute> lightInputLayout;
    RALVertexAttribute lightPosAttr;
    lightPosAttr.semantic = RALVertexSemantic::Position;
    lightPosAttr.format = RALVertexFormat::Float4;
    lightPosAttr.bufferSlot = 0;
    lightPosAttr.offset = 0;
    lightInputLayout.push_back(lightPosAttr);

    RALVertexAttribute lightUvAttr;
    lightUvAttr.semantic = RALVertexSemantic::TexCoord0;
    lightUvAttr.format = RALVertexFormat::Float2;
    lightUvAttr.bufferSlot = 0;
    lightUvAttr.offset = 16; // float4是16字节
    lightInputLayout.push_back(lightUvAttr);

    lightPipelineDesc.inputLayout = &lightInputLayout;
    lightPipelineDesc.rootSignature = m_lightRootSignature.Get();
    lightPipelineDesc.vertexShader = m_lightVertexShader.Get();
    lightPipelineDesc.pixelShader = m_lightPixelShader.Get();
    lightPipelineDesc.primitiveTopologyType = RALPrimitiveTopologyType::TriangleList;

    // 配置光栅化状态
    lightPipelineDesc.rasterizerState.cullMode = RALCullMode::None;
    lightPipelineDesc.rasterizerState.fillMode = RALFillMode::Solid;
    
    // 配置混合状态
    lightPipelineDesc.blendState.alphaToCoverageEnable = false;
    lightPipelineDesc.blendState.independentBlendEnable = false;
    RALRenderTargetBlendState lightBlendState;
    lightBlendState.blendEnable = false;
    lightBlendState.logicOpEnable = false;
    lightBlendState.colorWriteMask = 0xF;
    lightPipelineDesc.renderTargetBlendStates.push_back(lightBlendState);

    // 配置深度模板状态
    lightPipelineDesc.depthStencilState.depthEnable = false;
    lightPipelineDesc.depthStencilState.depthWriteMask = false;

    // 配置渲染目标格式
    lightPipelineDesc.numRenderTargets = 1;
    lightPipelineDesc.renderTargetFormats[0] = RALDataFormat::R8G8B8A8_UNorm;
    lightPipelineDesc.depthStencilFormat = RALDataFormat::D32_Float;

    // 创建光照阶段管道状态
    m_lightPipelineState = m_device->CreateGraphicsPipelineState(lightPipelineDesc);
    if (!m_lightPipelineState.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create light pass pipeline state");
        return false;
    }

    logDebug("[DEBUG] Scene::InitializeDeferredRendering succeeded");
    return true;
}

// 清理延迟着色相关资源
void Scene::CleanupDeferredRendering()
{
    // 清理GBuffer纹理和深度缓冲区
    m_gbufferA = nullptr;
    m_gbufferB = nullptr;
    m_gbufferC = nullptr;
    m_gbufferDepthStencil = nullptr;
    
    // 清理着色器和管道状态
    m_gbufferVertexShader = nullptr;
    m_gbufferPixelShader = nullptr;
    m_gbufferRootSignature = nullptr;
    m_gbufferPipelineState = nullptr;
    
    m_lightVertexShader = nullptr;
    m_lightPixelShader = nullptr;
    m_lightRootSignature = nullptr;
    m_lightPipelineState = nullptr;
    
    // 清理常量缓冲区
    m_lightPassConstBuffer = nullptr;
    
    // 清理全屏四边形
    m_fullscreenQuadVB = nullptr;
    m_fullscreenQuadIB = nullptr;
}

// 创建全屏四边形
void Scene::CreateFullscreenQuad()
{
    if (!m_device)
    {
        logDebug("[DEBUG] Scene::CreateFullscreenQuad failed: device is null");
        return;
    }

    // 定义全屏四边形顶点数据
    struct FullscreenQuadVertex
    {
        dx::XMFLOAT4 pos;
        dx::XMFLOAT2 uv;
    };

    FullscreenQuadVertex vertices[4] = 
    {
        { { -1.0f,  1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // 左上
        { {  1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }, // 右上
        { {  1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }, // 右下
        { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } }  // 左下
    };

    // 创建顶点缓冲区
    m_fullscreenQuadVB = m_device->CreateVertexBuffer(sizeof(vertices), sizeof(FullscreenQuadVertex), true);
    if (m_fullscreenQuadVB.Get())
    {
        m_device->UploadBuffer(m_fullscreenQuadVB.Get(), reinterpret_cast<const char*>(vertices), sizeof(vertices));
    }

    // 定义索引数据
    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    // 创建索引缓冲区
    m_fullscreenQuadIB = m_device->CreateIndexBuffer(6, true, true);
    if (m_fullscreenQuadIB.Get())
    {
        m_device->UploadBuffer(m_fullscreenQuadIB.Get(), reinterpret_cast<const char*>(indices), sizeof(indices));
    }
}

// 执行几何阶段
void Scene::ExecuteGeometryPass(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();

    // 设置渲染目标和深度模板
    IRALRenderTarget* renderTargets[3] = { m_gbufferA.Get(), m_gbufferB.Get(), m_gbufferC.Get() };
    commandList->SetRenderTargets(3, renderTargets, m_gbufferDepthStencil.Get());

    // 清除渲染目标和深度模板
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTarget(m_gbufferA.Get(), clearColor);
    commandList->ClearRenderTarget(m_gbufferB.Get(), clearColor);
    commandList->ClearRenderTarget(m_gbufferC.Get(), clearColor);
    commandList->ClearDepthStencil(m_gbufferDepthStencil.Get(), 1.0f, 0);

    // 更新场景常量缓冲区
    UpdateSceneConstBuffer(commandList, viewMatrix, projectionMatrix);

    // 设置GBuffer根签名和管线状态
    commandList->SetGraphicsRootSignature(m_gbufferRootSignature.Get());
    commandList->SetPipelineState(m_gbufferPipelineState.Get());

    // 设置根参数0（场景常量）
    commandList->SetGraphicsRootConstantBuffer(0, m_sceneConstBuffer.Get());

    // 设置图元拓扑
    commandList->SetPrimitiveTopology(RALPrimitiveTopologyType::TriangleList);

    // 渲染每个可见的Primitive对象
    for (size_t i = 0; i < m_primitives.size(); ++i)
    {
        auto& primitiveInfo = m_primitives[i];

        if (primitiveInfo.primitive && primitiveInfo.visible)
        {
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

// 执行光照阶段
void Scene::ExecuteLightingPass()
{
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();

    // 清空渲染目标，准备光照阶段
    commandList->SetRenderTargets(0, nullptr, nullptr);

    // 更新光照阶段常量缓冲区
    LightPassConstBuffer lightData;
    lightData.lightPos = m_lightPosition;
    lightData.lightDiffuseColor = m_lightDiffuseColor;
    lightData.lightSpecularColor = m_lightSpecularColor;
    lightData.padding1 = 0.0f;

    // 更新光照常量缓冲区
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    m_lightPassConstBuffer->Map(&mappedData);
    memcpy(mappedData, &lightData, sizeof(LightPassConstBuffer));
    m_lightPassConstBuffer->Unmap();

    // 设置光照阶段根签名和管线状态
    commandList->SetGraphicsRootSignature(m_lightRootSignature.Get());
    commandList->SetPipelineState(m_lightPipelineState.Get());

    // 设置根参数0（光照常量）
    commandList->SetGraphicsRootConstantBuffer(0, m_lightPassConstBuffer.Get());

    // 将GBuffer纹理设置为着色器资源
    // 注意：这里使用根描述符表来绑定多个着色器资源
    // 简化实现：直接绑定光照常量缓冲区
    commandList->SetGraphicsRootShaderResource(1, m_lightPassConstBuffer.Get());

    // 设置全屏四边形
    IRALVertexBuffer* vertexBuffer = m_fullscreenQuadVB.Get();
    commandList->SetVertexBuffers(0, 1, &vertexBuffer);
    commandList->SetIndexBuffer(m_fullscreenQuadIB.Get());
    commandList->SetPrimitiveTopology(RALPrimitiveTopologyType::TriangleList);

    // 绘制全屏四边形
    commandList->DrawIndexed(6, 1, 0, 0, 0);
}