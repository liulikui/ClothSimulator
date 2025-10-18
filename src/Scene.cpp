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
    dx::XMFLOAT4X4 invViewProj;     // 视图-投影矩阵的逆矩阵
    dx::XMFLOAT3 lightPos;          // 光源位置
    float padding1;                 // 4字节对齐填充
    dx::XMFLOAT4 lightDiffuseColor; // 光源漫反射颜色
    dx::XMFLOAT4 lightSpecularColor; // 光源高光颜色
    dx::XMFLOAT3 lightDirection;    // 光源方向
    float padding2;                 // 4字节对齐填充
    dx::XMFLOAT4 lightAmbientColor; // 环境光颜色
};

struct ObjectConstBuffer
{
    dx::XMFLOAT4X4 World;           // 世界矩阵
    dx::XMFLOAT3 diffuseColor;      // 漫反射颜色
    float padding1;                 // 4字节对齐填充
};

Scene::Scene()
    : m_device(nullptr)
    , m_backgroundColor({0.9f, 0.9f, 0.9f, 1.0f})
    , m_lightPosition({10.0f, 10.0f, 10.0f})
    , m_lightDirection({-1.0f, -1.0f, -1.0f})
    , m_lightDiffuseColor({1.0f, 1.0f, 1.0f, 1.0f})
    , m_lightSpecularColor({1.0f, 1.0f, 1.0f, 1.0f})
    , m_lightAmbientColor({0.1f, 0.1f, 0.1f, 1.0f})
{
    // 初始化场景
    // cameraConstBuffer将在渲染器中创建并传入
    // 对默认光源方向进行归一化
    dx::XMVECTOR dir = dx::XMVector3Normalize(dx::XMLoadFloat3(&m_lightDirection));
    dx::XMStoreFloat3(&m_lightDirection, dir);
    
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
    
    m_sceneConstBuffer = pDevice->CreateConstBuffer(sizeof(SceneConstBuffer), L"SceneConstBuffer");
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

    // 执行延迟着色的三个主要阶段
    // 1. 几何阶段：渲染场景到GBuffer
    ExecuteGeometryPass(viewMatrix, projectionMatrix);
    
    // 2. 光照阶段：使用GBuffer中的信息进行光照计算
    ExecuteLightingPass();
    
    // 3. Resolve阶段：将光照结果与材质信息结合，输出最终HDR场景颜色
    ExecuteResolvePass();
    
    // 4. 色调映射阶段：将HDR场景颜色转换为LDR并输出到backbuffer
    ExecuteTonemappingPass();
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

// 设置场景的光源方向（自动归一化）
void Scene::SetLightDirection(const dx::XMFLOAT3& direction)
{
    // 对传入的方向向量进行归一化
    dx::XMVECTOR dir = dx::XMVector3Normalize(dx::XMLoadFloat3(&direction));
    dx::XMStoreFloat3(&m_lightDirection, dir);
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
        primitiveInfo.constBuffer = m_device->CreateConstBuffer(sizeof(ObjectConstBuffer), L"ObjectConstBuffer");

        // 添加对象到场景中
        m_primitives.push_back(primitiveInfo);
    }

    m_addPrimitiveRequests.clear();
}

void Scene::UpdateSceneConstBuffer(IRALGraphicsCommandList* commandList, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    // 计算视图-投影矩阵
    dx::XMMATRIX viewProjMatrix = viewMatrix * projectionMatrix;
    // 计算视图-投影矩阵的逆矩阵
    dx::XMMATRIX invViewProjMatrix = dx::XMMatrixInverse(nullptr, viewProjMatrix);

    SceneConstBuffer data;
    dx::XMStoreFloat4x4(&data.View, dx::XMMatrixTranspose(viewMatrix));
    dx::XMStoreFloat4x4(&data.Proj, dx::XMMatrixTranspose(projectionMatrix));
    dx::XMStoreFloat4x4(&data.ViewProj, dx::XMMatrixTranspose(viewProjMatrix));
    dx::XMStoreFloat4x4(&data.invViewProj, dx::XMMatrixTranspose(invViewProjMatrix));
    data.lightPos = m_lightPosition;
    data.lightDiffuseColor = m_lightDiffuseColor;
    data.lightSpecularColor = m_lightSpecularColor;
    data.lightDirection = m_lightDirection;
    data.lightAmbientColor = m_lightAmbientColor;
    data.padding1 = 0.0f;
    data.padding2 = 0.0f;

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

bool Scene::CreateRenderingResources(uint32_t width, uint32_t height)
{
    // 创建GBuffer纹理
    // GBufferA: 世界空间法线 (RGBA16F用于精度)
    m_gbufferA = m_device->CreateRenderTarget(width, height, RALDataFormat::R16G16B16A16_Float, L"GBufferA_Normals");
    if (!m_gbufferA.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferA");
        return false;
    }
    
    // 创建GBufferA的RTV和SRV
    RALRenderTargetViewDesc rtvDescA;
    rtvDescA.format = RALDataFormat::R16G16B16A16_Float;
    m_gbufferARTV = m_device->CreateRenderTargetView(m_gbufferA.Get(), rtvDescA, L"GBufferA_RTV");
    if (!m_gbufferARTV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferA RTV");
        return false;
    }
    
    RALShaderResourceViewDesc srvDescA;
    srvDescA.format = RALDataFormat::R16G16B16A16_Float;
    m_gbufferASRV = m_device->CreateShaderResourceView(m_gbufferA.Get(), srvDescA, L"GBufferA_SRV");
    if (!m_gbufferASRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferA SRV");
        return false;
    }

    // GBufferB: Metallic (R), Specular (G), Roughness (B) (RGB8_UNorm)
    m_gbufferB = m_device->CreateRenderTarget(width, height, RALDataFormat::R8G8B8A8_UNorm, L"GBufferB_MetallicSpecRough");
    if (!m_gbufferB.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferB");
        return false;
    }
    
    // 创建GBufferB的RTV和SRV
    RALRenderTargetViewDesc rtvDescB;
    rtvDescB.format = RALDataFormat::R8G8B8A8_UNorm;
    m_gbufferBRTV = m_device->CreateRenderTargetView(m_gbufferB.Get(), rtvDescB, L"GBufferB_RTV");
    if (!m_gbufferBRTV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferB RTV");
        return false;
    }
    
    RALShaderResourceViewDesc srvDescB;
    srvDescB.format = RALDataFormat::R8G8B8A8_UNorm;
    m_gbufferBSRV = m_device->CreateShaderResourceView(m_gbufferB.Get(), srvDescB, L"GBufferB_SRV");
    if (!m_gbufferBSRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferB SRV");
        return false;
    }

    // GBufferC: BaseColor RGB (RGB8_UNorm)
    m_gbufferC = m_device->CreateRenderTarget(width, height, RALDataFormat::R8G8B8A8_UNorm, L"GBufferC_BaseColor");
    if (!m_gbufferC.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferC");
        return false;
    }
    
    // 创建GBufferC的RTV和SRV
    RALRenderTargetViewDesc rtvDescC;
    rtvDescC.format = RALDataFormat::R8G8B8A8_UNorm;
    m_gbufferCRTV = m_device->CreateRenderTargetView(m_gbufferC.Get(), rtvDescC, L"GBufferC_RTV");
    if (!m_gbufferCRTV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferC RTV");
        return false;
    }
    
    RALShaderResourceViewDesc srvDescC;
    srvDescC.format = RALDataFormat::R8G8B8A8_UNorm;
    m_gbufferCSRV = m_device->CreateShaderResourceView(m_gbufferC.Get(), srvDescC, L"GBufferC_SRV");
    if (!m_gbufferCSRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create GBufferC SRV");
        return false;
    }

    // 创建深度/模板缓冲区
    m_gbufferDepthStencil = m_device->CreateDepthStencil(width, height, RALDataFormat::R32_Typeless, L"GBuffer_DepthStencil");
    if (!m_gbufferDepthStencil.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create depth stencil");
        return false;
    }
    
    // 创建深度模板缓冲区的DSV和SRV
    RALDepthStencilViewDesc dsvDesc;
    dsvDesc.format = RALDataFormat::D32_Float;
    m_gbufferDSV = m_device->CreateDepthStencilView(m_gbufferDepthStencil.Get(), dsvDesc, L"GBuffer_DepthStencil_DSV");
    if (!m_gbufferDSV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create depth stencil DSV");
        return false;
    }
    
    RALShaderResourceViewDesc depthSRVDesc;
    depthSRVDesc.format = RALDataFormat::R32_Float;
    m_gbufferDepthSRV = m_device->CreateShaderResourceView(m_gbufferDepthStencil.Get(), depthSRVDesc, L"GBuffer_DepthStencil_SRV");
    if (!m_gbufferDepthSRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create depth stencil SRV");
        return false;
    }

    // 创建光照结果RT - Diffuse光照结果 (R16G16B16A16_UNorm)
    m_diffuseLightRT = m_device->CreateRenderTarget(width, height, RALDataFormat::R16G16B16A16_UNorm, L"DiffuseLightRT");
    if (!m_diffuseLightRT.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create diffuse light RT");
        return false;
    }
    
    // 创建Diffuse光照结果的RTV和SRV
    RALRenderTargetViewDesc diffuseRTVDESC;
    diffuseRTVDESC.format = RALDataFormat::R16G16B16A16_UNorm;
    m_diffuseLightRTV = m_device->CreateRenderTargetView(m_diffuseLightRT.Get(), diffuseRTVDESC, L"DiffuseLight_RTV");
    if (!m_diffuseLightRTV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create diffuse light RTV");
        return false;
    }
    
    RALShaderResourceViewDesc diffuseSRVDesc;
    diffuseSRVDesc.format = RALDataFormat::R16G16B16A16_UNorm;
    m_diffuseLightSRV = m_device->CreateShaderResourceView(m_diffuseLightRT.Get(), diffuseSRVDesc, L"DiffuseLight_SRV");
    if (!m_diffuseLightSRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create diffuse light SRV");
        return false;
    }

    // 创建光照结果RT - Specular光照结果 (R16G16B16A16_UNorm)
    m_specularLightRT = m_device->CreateRenderTarget(width, height, RALDataFormat::R16G16B16A16_UNorm, L"SpecularLightRT");
    if (!m_specularLightRT.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create specular light RT");
        return false;
    }
    
    // 创建Specular光照结果的RTV和SRV
    RALRenderTargetViewDesc specularRTVDESC;
    specularRTVDESC.format = RALDataFormat::R16G16B16A16_UNorm;
    m_specularLightRTV = m_device->CreateRenderTargetView(m_specularLightRT.Get(), specularRTVDESC, L"SpecularLight_RTV");
    if (!m_specularLightRTV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create specular light RTV");
        return false;
    }
    
    RALShaderResourceViewDesc specularSRVDesc;
    specularSRVDesc.format = RALDataFormat::R16G16B16A16_UNorm;
    m_specularLightSRV = m_device->CreateShaderResourceView(m_specularLightRT.Get(), specularSRVDesc, L"SpecularLight_SRV");
    if (!m_specularLightSRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create specular light SRV");
        return false;
    }

    // 创建HDR场景颜色渲染目标（用于延迟着色Resolve结果）
    m_HDRSceneColor = m_device->CreateRenderTarget(width, height, RALDataFormat::R16G16B16A16_UNorm, L"HDRSceneColor");
    if (!m_HDRSceneColor.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create HDR scene color render target");
        return false;
    }

    // 创建HDR场景颜色RTV和SRV
    RALRenderTargetViewDesc hdrRTVDesc;
    hdrRTVDesc.format = RALDataFormat::R16G16B16A16_UNorm;
    m_HDRSceneColorRTV = m_device->CreateRenderTargetView(m_HDRSceneColor.Get(), hdrRTVDesc, L"HDRSceneColor_RTV");
    if (!m_HDRSceneColorRTV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create HDR scene color RTV");
        return false;
    }
    
    RALShaderResourceViewDesc hdrSRVDesc;
    hdrSRVDesc.format = RALDataFormat::R16G16B16A16_UNorm;
    m_HDRSceneColorSRV = m_device->CreateShaderResourceView(m_HDRSceneColor.Get(), hdrSRVDesc, L"HDRSceneColor_SRV");
    if (!m_HDRSceneColorSRV.Get())
    {
        logDebug("[DEBUG] Scene::CreateRenderingResources failed: failed to create HDR scene color SRV");
        return false;
    }

    return true;
}

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
    
    // 创建渲染资源
    if (!CreateRenderingResources(width, height))
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: CreateRenderingResources failed");
        return false;
    }

    // 创建光照阶段常量缓冲区
    m_lightPassConstBuffer = m_device->CreateConstBuffer(sizeof(LightPassConstBuffer), L"LightPassConstBuffer");
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
        RALRootSignatureFlags::AllowInputAssemblerInputLayout,
        L"GBufferRootSignature"
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
        "cbuffer SceneConstants : register(b0) {\n"
        "   float4x4 View;\n"
        "   float4x4 Proj;\n"
        "   float4x4 ViewProj;\n"
        "   float4x4 invViewProj;\n"
        "   float3 lightPos;\n"
        "   float padding1;\n"
        "   float4 lightDiffuseColor;\n"
        "   float4 lightSpecularColor;\n"
        "   float3 lightDirection;\n"
        "   float padding2;\n"
        "   float4 lightAmbientColor;\n"
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
    m_gbufferPipelineState = m_device->CreateGraphicsPipelineState(gbufferPipelineDesc, L"GBufferPipelineState");
    if (!m_gbufferPipelineState.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create GBuffer pipeline state");
        return false;
    }

    // 创建光照阶段根签名
    std::vector<RALRootParameter> lightRootParameters(5);
    // 根参数0：场景常量缓冲区（包含invViewProj和光照信息）
    InitAsConstantBufferView(lightRootParameters[0], 0, 0, RALShaderVisibility::Pixel);
    
    // 使用4个描述符表，对应三个GBuffer纹理和一个深度纹理
    // 根参数1：GBufferA描述符表
    std::vector<RALRootDescriptorTableRange> range1;
    RALRootDescriptorTableRange gbufferARange;
    gbufferARange.Type = RALDescriptorRangeType::SRV;
    gbufferARange.NumDescriptors = 1; // 1个GBuffer纹理
    gbufferARange.BaseShaderRegister = 0;
    gbufferARange.RegisterSpace = 0;
    range1.push_back(gbufferARange);
    InitAsDescriptorTable(lightRootParameters[1], range1, RALShaderVisibility::Pixel);
    
    // 根参数2：GBufferB描述符表
    std::vector<RALRootDescriptorTableRange> range2;
    RALRootDescriptorTableRange gbufferBRange;
    gbufferBRange.Type = RALDescriptorRangeType::SRV;
    gbufferBRange.NumDescriptors = 1; // 1个GBuffer纹理
    gbufferBRange.BaseShaderRegister = 1;
    gbufferBRange.RegisterSpace = 0;
    range2.push_back(gbufferBRange);
    InitAsDescriptorTable(lightRootParameters[2], range2, RALShaderVisibility::Pixel);
    
    // 根参数3：GBufferC描述符表
    std::vector<RALRootDescriptorTableRange> range3;
    RALRootDescriptorTableRange gbufferCRange;
    gbufferCRange.Type = RALDescriptorRangeType::SRV;
    gbufferCRange.NumDescriptors = 1; // 1个GBuffer纹理
    gbufferCRange.BaseShaderRegister = 2;
    gbufferCRange.RegisterSpace = 0;
    range3.push_back(gbufferCRange);
    InitAsDescriptorTable(lightRootParameters[3], range3, RALShaderVisibility::Pixel);
    
    // 根参数4：深度纹理描述符表
    std::vector<RALRootDescriptorTableRange> range4;
    RALRootDescriptorTableRange depthRange;
    depthRange.Type = RALDescriptorRangeType::SRV;
    depthRange.NumDescriptors = 1; // 1个深度纹理
    depthRange.BaseShaderRegister = 3;
    depthRange.RegisterSpace = 0;
    range4.push_back(depthRange);
    InitAsDescriptorTable(lightRootParameters[4], range4, RALShaderVisibility::Pixel);

    // 定义静态采样器（使用点采样）
    RALStaticSampler lightSampler;
    InitStaticSampler(
        lightSampler,
        RALFilter::MinMagMipPoint,
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
        RALRootSignatureFlags::AllowInputAssemblerInputLayout,
        L"LightPassRootSignature"
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
        "cbuffer SceneConstants : register(b0) {\n"
        "   float4x4 View;\n"
        "   float4x4 Proj;\n"
        "   float4x4 ViewProj;\n"
        "   float4x4 invViewProj;\n"
        "   float3 lightPos;\n"
        "   float padding1;\n"
        "   float4 lightDiffuseColor;\n"
        "   float4 lightSpecularColor;\n"
        "   float3 lightDirection;\n"
        "   float padding2;\n"
        "   float4 lightAmbientColor;\n"
        "};\n"
        "// 采样GBuffer纹理\n"
        "Texture2D<float4> gbufferA : register(t0);\n"
        "Texture2D<float4> gbufferB : register(t1);\n"
        "Texture2D<float4> gbufferC : register(t2);\n"
        "Texture2D<float> depthTexture : register(t3);\n"
        "SamplerState samplerGBuffer : register(s0);\n"
        "// 输出到两个渲染目标：Diffuse和Specular光照结果\n"
        "struct PS_OUTPUT {\n"
        "   float4 diffuseResult : SV_TARGET0; // Diffuse光照结果（不乘材质）\n"
        "   float4 specularResult : SV_TARGET1; // Specular光照结果（不乘材质）\n"
        "};\n"
        "// 从屏幕空间重建世界空间位置\n"
        "float3 ReconstructWorldPosition(float2 uv, float depth) {\n"
        "   // 将深度值从[0,1]范围转换到NDC空间\n"
        "   float4 ndcPos = float4(uv * 2.0f - 1.0f, depth * 2.0f - 1.0f, 1.0f);\n"
        "   \n"
        "   // 使用ViewProj的逆矩阵将NDC坐标转换到世界空间\n"
        "   float4 worldPos = mul(ndcPos, invViewProj);\n"
        "   \n"
        "   // 透视除法\n"
        "   worldPos.xyz /= worldPos.w;\n"
        "   \n"
        "   return worldPos.xyz;\n"
        "}\n"
        "PS_OUTPUT main(PS_INPUT input) {\n"
        "   PS_OUTPUT output;\n"
        "   // 初始化输出\n"
        "   output.diffuseResult = float4(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "   output.specularResult = float4(0.0f, 0.0f, 0.0f, 1.0f);\n"
        "   \n"
        "   // 从GBuffer中采样数据\n"
        "   float4 normalSample = gbufferA.Sample(samplerGBuffer, input.uv);\n"
        "   // 从深度纹理采样深度值\n"
        "   float depth = depthTexture.Sample(samplerGBuffer, input.uv).r;\n"
        "   \n"
        "   // 检查是否为背景像素\n"
        "   if (depth >= 1.0f - 1e-6f) {\n"
        "       return output; // 如果是背景，返回黑色\n"
        "   }\n"
        "   \n"
        "   // 从[0,1]映射回[-1,1]并归一化法线\n"
        "   float3 normal = (normalSample.xyz * 2.0f) - 1.0f;\n"
        "   normal = normalize(normal);\n"
        "   \n"
        "   // 重建世界空间位置\n"
        "   float3 worldPos = ReconstructWorldPosition(input.uv, depth);\n"
        "   \n"
        "   // 计算视图方向（从世界空间点指向摄像机）\n"
        "   // 注意：这里需要使用摄像机的世界位置，为简化可以从View矩阵中提取或传入常量\n"
        "   // 为了简化，我们可以从View矩阵的最后一行提取摄像机位置\n"
        "   float3 viewDir = normalize(float3(0.0f, 0.0f, 0.0f) - worldPos); // 假设摄像机在原点\n"
        "   \n"
        "   // 使用场景中的光源方向（已经归一化）\n"
        "   float3 lightDir = -normalize(lightDirection); // 注意：lightDirection是指向光源的方向\n"
        "   \n"
        "   // 计算半程向量（Blinn-Phong模型）\n"
        "   float3 halfVec = normalize(lightDir + viewDir);\n"
        "   \n"
        "   // 计算漫反射分量（不乘材质）\n"
        "   float diffuseTerm = max(dot(normal, lightDir), 0.0f);\n"
        "   // 计算高光分量（不乘材质，使用Blinn-Phong模型，假设shininess为32）\n"
        "   float specularTerm = pow(max(dot(normal, halfVec), 0.0f), 32.0f);\n"
        "   \n"
        "   // 输出漫反射光照结果（不乘材质）\n"
        "   output.diffuseResult.rgb = diffuseTerm * lightDiffuseColor.rgb;\n"
        "   output.diffuseResult.a = 1.0f;\n"
        "   \n"
        "   // 输出高光光照结果（不乘材质）\n"
        "   output.specularResult.rgb = specularTerm * lightSpecularColor.rgb;\n"
        "   output.specularResult.a = 1.0f;\n"
        "   \n"
        "   return output;\n"
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
    // 添加两个混合状态，与渲染目标数量匹配
    lightPipelineDesc.renderTargetBlendStates.push_back(lightBlendState);
    lightPipelineDesc.renderTargetBlendStates.push_back(lightBlendState);

    // 配置深度模板状态
    lightPipelineDesc.depthStencilState.depthEnable = false;
    lightPipelineDesc.depthStencilState.depthWriteMask = false;

    // 配置渲染目标格式（支持两个渲染目标）
    lightPipelineDesc.numRenderTargets = 2;
    lightPipelineDesc.renderTargetFormats[0] = RALDataFormat::R16G16B16A16_UNorm; // Diffuse光照结果
    lightPipelineDesc.renderTargetFormats[1] = RALDataFormat::R16G16B16A16_UNorm; // Specular光照结果
    lightPipelineDesc.depthStencilFormat = RALDataFormat::D32_Float;

    // 创建光照阶段管道状态
    m_lightPipelineState = m_device->CreateGraphicsPipelineState(lightPipelineDesc, L"LightPassPipelineState");
    if (!m_lightPipelineState.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create light pass pipeline state");
        return false;
    }

    // 创建Resolve阶段根签名
    std::vector<RALRootParameter> resolveRootParameters(5);
    // 根参数0：场景常量缓冲区（包含光照信息）
    InitAsConstantBufferView(resolveRootParameters[0], 0, 0, RALShaderVisibility::Pixel);
    
    // 使用4个描述符表，对应两个光照结果RT和GBufferB（材质信息）
    // 根参数1：Diffuse光照结果描述符表
    std::vector<RALRootDescriptorTableRange> diffuseLightRange;
    RALRootDescriptorTableRange diffuseLightSRVRange;
    diffuseLightSRVRange.Type = RALDescriptorRangeType::SRV;
    diffuseLightSRVRange.NumDescriptors = 1; // 1个Diffuse光照结果纹理
    diffuseLightSRVRange.BaseShaderRegister = 0;
    diffuseLightSRVRange.RegisterSpace = 0;
    diffuseLightRange.push_back(diffuseLightSRVRange);
    InitAsDescriptorTable(resolveRootParameters[1], diffuseLightRange, RALShaderVisibility::Pixel);
    
    // 根参数2：Specular光照结果描述符表
    std::vector<RALRootDescriptorTableRange> specularLightRange;
    RALRootDescriptorTableRange specularLightSRVRange;
    specularLightSRVRange.Type = RALDescriptorRangeType::SRV;
    specularLightSRVRange.NumDescriptors = 1; // 1个Specular光照结果纹理
    specularLightSRVRange.BaseShaderRegister = 1;
    specularLightSRVRange.RegisterSpace = 0;
    specularLightRange.push_back(specularLightSRVRange);
    InitAsDescriptorTable(resolveRootParameters[2], specularLightRange, RALShaderVisibility::Pixel);
    
    // 根参数3：GBufferB描述符表（包含材质信息）
    std::vector<RALRootDescriptorTableRange> materialRange;
    RALRootDescriptorTableRange materialSRVRange;
    materialSRVRange.Type = RALDescriptorRangeType::SRV;
    materialSRVRange.NumDescriptors = 1; // 1个材质信息纹理
    materialSRVRange.BaseShaderRegister = 2;
    materialSRVRange.RegisterSpace = 0;
    materialRange.push_back(materialSRVRange);
    InitAsDescriptorTable(resolveRootParameters[3], materialRange, RALShaderVisibility::Pixel);
    
    // 根参数4：GBufferC描述符表（包含BaseColor信息）
    std::vector<RALRootDescriptorTableRange> baseColorRange;
    RALRootDescriptorTableRange baseColorSRVRange;
    baseColorSRVRange.Type = RALDescriptorRangeType::SRV;
    baseColorSRVRange.NumDescriptors = 1; // 1个BaseColor纹理
    baseColorSRVRange.BaseShaderRegister = 3;
    baseColorSRVRange.RegisterSpace = 0;
    baseColorRange.push_back(baseColorSRVRange);
    InitAsDescriptorTable(resolveRootParameters[4], baseColorRange, RALShaderVisibility::Pixel);

    // 定义静态采样器（使用点采样）
    RALStaticSampler resolveSampler;
    InitStaticSampler(
        resolveSampler,
        RALFilter::MinMagMipPoint,
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

    std::vector<RALStaticSampler> resolveSamplers = { resolveSampler };

    // 创建Resolve阶段根签名
    m_resolveRootSignature = m_device->CreateRootSignature(
        resolveRootParameters,
        resolveSamplers,
        RALRootSignatureFlags::AllowInputAssemblerInputLayout,
        L"ResolveRootSignature"
    );
    if (!m_resolveRootSignature.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create resolve pass root signature");
        return false;
    }

    // 定义Resolve阶段着色器代码
    const char* resolveVSCode = 
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

    const char* resolvePSCode = 
        "struct PS_INPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "\n"
        "// 场景常量缓冲区\n"
        "cbuffer SceneConstants : register(b0) {\n"
        "   float4x4 View;\n"
        "   float4x4 Proj;\n"
        "   float4x4 ViewProj;\n"
        "   float4x4 invViewProj;\n"
        "   float3 lightPos;\n"
        "   float padding1;\n"
        "   float4 lightDiffuseColor;\n"
        "   float4 lightSpecularColor;\n"
        "   float3 lightDirection;\n"
        "   float padding2;\n"
        "   float4 lightAmbientColor;  // 环境光颜色\n"
        "}\n\n"
        "// 采样纹理\n"
        "Texture2D<float4> diffuseLightTexture : register(t0);\n"
        "Texture2D<float4> specularLightTexture : register(t1);\n"
        "Texture2D<float4> materialTexture : register(t2);\n"
        "Texture2D<float4> baseColorTexture : register(t3);\n"
        "\n"
        "SamplerState samplerResolve : register(s0);\n"
        "\n"
        "// 输出到一个渲染目标：HDR场景颜色\n"
        "struct PS_OUTPUT {\n"
        "   float4 hdrColor : SV_TARGET0;\n"
        "};\n"
        "\n"
        "PS_OUTPUT main(PS_INPUT input) {\n"
        "   PS_OUTPUT output;\n"
        "   \n"
        "   // 采样光照结果和材质信息\n"
        "   float4 diffuseLight = diffuseLightTexture.Sample(samplerResolve, input.uv);\n"
        "   float4 specularLight = specularLightTexture.Sample(samplerResolve, input.uv);\n"
        "   float4 material = materialTexture.Sample(samplerResolve, input.uv);\n"
        "   float4 baseColor = baseColorTexture.Sample(samplerResolve, input.uv);\n"
        "   \n"
        "   // 从GBufferB提取材质信息\n"
        "   float metallic = material.r;\n"
        "   float specular = material.g;\n"
        "   float roughness = material.b;\n"
        "   \n"
        "   // 计算最终的HDR场景颜色\n"
        "   // 环境光部分：环境光 * 基础颜色\n"
        "   // 漫反射部分：漫反射光照 * 基础颜色\n"
        "   // 高光部分：高光光照 * 材质高光颜色 * 金属度因素\n"
        "   float3 ambient = lightAmbientColor.rgb * baseColor.rgb;\n"
        "   float3 diffuse = diffuseLight.rgb * baseColor.rgb;\n"
        "   float3 specularColor = specularLight.rgb * specular * (metallic * baseColor.rgb + (1.0 - metallic) * float3(0.04, 0.04, 0.04));\n"
        "   \n"
        "   // 合并环境光、漫反射和高光结果\n"
        "   output.hdrColor.rgb = ambient + diffuse + specularColor;\n"
        "   output.hdrColor.a = 1.0f;\n"
        "   \n"
        "   return output;\n"
        "}";

    // 编译Resolve阶段着色器
    m_resolveVertexShader = m_device->CompileVertexShader(resolveVSCode, "main");
    if (!m_resolveVertexShader.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile resolve pass vertex shader");
        return false;
    }

    m_resolvePixelShader = m_device->CompilePixelShader(resolvePSCode, "main");
    if (!m_resolvePixelShader.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile resolve pass pixel shader");
        return false;
    }

    // 创建Resolve阶段管道状态
    RALGraphicsPipelineStateDesc resolvePipelineDesc = {};

    // 配置输入布局 (全屏四边形)
    std::vector<RALVertexAttribute> resolveInputLayout;
    RALVertexAttribute resolvePosAttr;
    resolvePosAttr.semantic = RALVertexSemantic::Position;
    resolvePosAttr.format = RALVertexFormat::Float4;
    resolvePosAttr.bufferSlot = 0;
    resolvePosAttr.offset = 0;
    resolveInputLayout.push_back(resolvePosAttr);

    RALVertexAttribute resolveUvAttr;
    resolveUvAttr.semantic = RALVertexSemantic::TexCoord0;
    resolveUvAttr.format = RALVertexFormat::Float2;
    resolveUvAttr.bufferSlot = 0;
    resolveUvAttr.offset = 16; // float4是16字节
    resolveInputLayout.push_back(resolveUvAttr);

    resolvePipelineDesc.inputLayout = &resolveInputLayout;
    resolvePipelineDesc.rootSignature = m_resolveRootSignature.Get();
    resolvePipelineDesc.vertexShader = m_resolveVertexShader.Get();
    resolvePipelineDesc.pixelShader = m_resolvePixelShader.Get();
    resolvePipelineDesc.primitiveTopologyType = RALPrimitiveTopologyType::TriangleList;

    // 配置光栅化状态
    resolvePipelineDesc.rasterizerState.cullMode = RALCullMode::None;
    resolvePipelineDesc.rasterizerState.fillMode = RALFillMode::Solid;
    
    // 配置混合状态
    resolvePipelineDesc.blendState.alphaToCoverageEnable = false;
    resolvePipelineDesc.blendState.independentBlendEnable = false;
    RALRenderTargetBlendState resolveBlendState;
    resolveBlendState.blendEnable = false;
    resolveBlendState.logicOpEnable = false;
    resolveBlendState.colorWriteMask = 0xF;
    resolvePipelineDesc.renderTargetBlendStates.push_back(resolveBlendState);

    // 配置深度模板状态
    resolvePipelineDesc.depthStencilState.depthEnable = false;
    resolvePipelineDesc.depthStencilState.depthWriteMask = false;

    // 配置渲染目标格式（单一渲染目标 - HDR场景颜色）
    resolvePipelineDesc.numRenderTargets = 1;
    resolvePipelineDesc.renderTargetFormats[0] = RALDataFormat::R16G16B16A16_UNorm; // HDR场景颜色
    resolvePipelineDesc.depthStencilFormat = RALDataFormat::D32_Float;

    // 创建Resolve阶段管道状态
    m_resolvePipelineState = m_device->CreateGraphicsPipelineState(resolvePipelineDesc, L"ResolvePipelineState");
    if (!m_resolvePipelineState.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create resolve pass pipeline state");
        return false;
    }

    // 创建色调映射根签名
    std::vector<RALRootParameter> tonemappingRootParameters;
    
    // 创建采样器描述符表 - 移除这个，因为我们已经使用了静态采样器
    
    // 创建着色器资源视图描述符表
    RALRootParameter srvTable;
    srvTable.Type = RALRootParameterType::DescriptorTable;
    srvTable.ShaderVisibility = RALShaderVisibility::Pixel;
    
    RALRootDescriptorTableRange srvRange(RALDescriptorRangeType::SRV, 1, 0, 0);
    srvTable.Data.DescriptorTable.Ranges.push_back(srvRange);
    tonemappingRootParameters.push_back(srvTable);
    
    // 创建静态采样器
    std::vector<RALStaticSampler> tonemappingStaticSamplers;
    RALStaticSampler sampler;
    sampler.Filter = RALFilter::MinMagMipLinear;
    sampler.AddressU = RALTextureAddressMode::Wrap;
    sampler.AddressV = RALTextureAddressMode::Wrap;
    sampler.AddressW = RALTextureAddressMode::Wrap;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = RALComparisonFunc::Never;
    sampler.BorderColor = RALStaticBorderColor::TransparentBlack;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = 32.0f;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    
    tonemappingStaticSamplers.push_back(sampler);
    
    // 创建根签名
    m_tonemappingRootSignature = m_device->CreateRootSignature(
        tonemappingRootParameters,
        tonemappingStaticSamplers,
        RALRootSignatureFlags::AllowInputAssemblerInputLayout,
        L"TonemappingRootSignature"
    );
    
    if (!m_tonemappingRootSignature.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create tonemapping root signature");
        return false;
    }
    
    // 编译色调映射顶点着色器
    const char* tonemappingVSCode = 
        "struct VS_INPUT {\n"
        "   float4 pos : POSITION;\n"
        "   float2 uv : TEXCOORD0;\n"
        "};\n"
        "\n"
        "struct VS_OUTPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "\n"
        "VS_OUTPUT main(VS_INPUT input) {\n"
        "   VS_OUTPUT output;\n"
        "   output.pos = input.pos;\n"
        "   output.uv = input.uv;\n"
        "   return output;\n"
        "};";
    
    m_tonemappingVS = m_device->CompileVertexShader(tonemappingVSCode);
    if (!m_tonemappingVS.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile tonemapping vertex shader");
        return false;
    }
    
    // 编译色调映射像素着色器（使用ACES色调映射曲线）
    const char* tonemappingPSCode = 
        "struct PS_INPUT {\n"
        "   float4 pos : SV_POSITION;\n"
        "   float2 uv : TEXCOORD;\n"
        "};\n"
        "\n"
        "Texture2D<float4> hdrSceneTexture : register(t0);\n"
        "SamplerState samplerTonemapping : register(s0);\n"
        "\n"
        "// ACES色调映射函数\n"
        "float3 ACESFilm(float3 x) {\n"
        "   float a = 2.51f;\n"
        "   float b = 0.03f;\n"
        "   float c = 2.43f;\n"
        "   float d = 0.59f;\n"
        "   float e = 0.14f;\n"
        "   return saturate((x * (a * x + b)) / (x * (c * x + d) + e));\n"
        "}\n"
        "\n"
        "struct PS_OUTPUT {\n"
        "   float4 ldrColor : SV_TARGET0;\n"
        "};\n"
        "\n"
        "PS_OUTPUT main(PS_INPUT input) {\n"
        "   PS_OUTPUT output;\n"
        "   \n"
        "   // 采样HDR场景纹理\n"
        "   float4 hdrColor = hdrSceneTexture.Sample(samplerTonemapping, input.uv);\n"
        "   \n"
        "   // 应用ACES色调映射\n"
        "   float3 ldrColor = ACESFilm(hdrColor.rgb);\n"
        "   \n"
        "   // 应用gamma校正（近似值）\n"
        "   ldrColor = pow(ldrColor, 1.0 / 2.2);\n"
        "   \n"
        "   output.ldrColor = float4(ldrColor, 1.0f);\n"
        "   return output;\n"
        "};";
    
    m_tonemappingPS = m_device->CompilePixelShader(tonemappingPSCode);
    if (!m_tonemappingPS.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to compile tonemapping pixel shader");
        return false;
    }
    
    // 创建色调映射管线状态
    RALGraphicsPipelineStateDesc tonemappingPsoDesc = {};
    tonemappingPsoDesc.rootSignature = m_tonemappingRootSignature.Get();
    tonemappingPsoDesc.vertexShader = m_tonemappingVS.Get();
    tonemappingPsoDesc.pixelShader = m_tonemappingPS.Get();
    
    // 设置输入布局 - 为全屏四边形创建输入布局
    std::vector<RALVertexAttribute> tonemappingInputLayout;
    RALVertexAttribute tonemappingPosAttr;
    tonemappingPosAttr.semantic = RALVertexSemantic::Position;
    tonemappingPosAttr.format = RALVertexFormat::Float4;
    tonemappingPosAttr.bufferSlot = 0;
    tonemappingPosAttr.offset = 0;
    tonemappingInputLayout.push_back(tonemappingPosAttr);

    RALVertexAttribute tonemappingUvAttr;
    tonemappingUvAttr.semantic = RALVertexSemantic::TexCoord0;
    tonemappingUvAttr.format = RALVertexFormat::Float2;
    tonemappingUvAttr.bufferSlot = 0;
    tonemappingUvAttr.offset = 16; // float4是16字节
    tonemappingInputLayout.push_back(tonemappingUvAttr);

    tonemappingPsoDesc.inputLayout = &tonemappingInputLayout;
    
    tonemappingPsoDesc.primitiveTopologyType = RALPrimitiveTopologyType::TriangleList;
    
    // 设置光栅化状态
    tonemappingPsoDesc.rasterizerState.fillMode = RALFillMode::Solid;
    tonemappingPsoDesc.rasterizerState.cullMode = RALCullMode::None;  // 无剔除，全屏四边形
    
    // 设置混合状态
    tonemappingPsoDesc.blendState.alphaToCoverageEnable = false;
    tonemappingPsoDesc.blendState.independentBlendEnable = false;
    
    // 设置渲染目标混合状态
    tonemappingPsoDesc.renderTargetBlendStates.resize(1);
    auto& rtBlendState = tonemappingPsoDesc.renderTargetBlendStates[0];
    rtBlendState.blendEnable = false;
    rtBlendState.logicOpEnable = false;
    rtBlendState.srcBlend = RALBlendFactor::One;
    rtBlendState.destBlend = RALBlendFactor::Zero;
    rtBlendState.blendOp = RALBlendOp::Add;
    rtBlendState.srcBlendAlpha = RALBlendFactor::One;
    rtBlendState.destBlendAlpha = RALBlendFactor::Zero;
    rtBlendState.blendOpAlpha = RALBlendOp::Add;
    rtBlendState.logicOp = RALLogicOp::Noop;
    rtBlendState.colorWriteMask = 0xf; // RGBA所有通道都写入
    
    // 深度模板状态使用默认值即可
    
    // 设置渲染目标格式
    tonemappingPsoDesc.numRenderTargets = 1;
    tonemappingPsoDesc.renderTargetFormats[0] = RALDataFormat::R8G8B8A8_UNorm;  // LDR格式
    tonemappingPsoDesc.depthStencilFormat = RALDataFormat::D32_Float;
    
    m_tonemappingPipelineState = m_device->CreateGraphicsPipelineState(tonemappingPsoDesc, L"TonemappingPipelineState");
    if (!m_tonemappingPipelineState.Get())
    {
        logDebug("[DEBUG] Scene::InitializeDeferredRendering failed: failed to create tonemapping pipeline state");
        return false;
    }
    
    logDebug("[DEBUG] Scene::InitializeDeferredRendering succeeded");
    return true;
}


void Scene::ExecuteTonemappingPass()
{
    // 获取backbuffer的渲染目标视图
    IRALRenderTargetView* backBufferRTV = m_device->GetBackBufferRTV();
    
    // 获取命令列表
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();
    
    // 设置渲染目标为backbuffer
    commandList->SetRenderTargets(1, &backBufferRTV, nullptr);
    
    // 清除渲染目标（可选，但为了干净的输出）
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTarget(backBufferRTV, clearColor);
    
    // 设置根签名
    commandList->SetGraphicsRootSignature(m_tonemappingRootSignature.Get());
    
    // 设置管线状态
    commandList->SetPipelineState(m_tonemappingPipelineState.Get());
    
    // 设置着色器资源视图到根描述符表
    commandList->SetGraphicsRootDescriptorTable(0, m_HDRSceneColorSRV.Get());
    
    // 设置顶点缓冲区
    IRALVertexBuffer* vertexBuffer = m_fullscreenQuadVB.Get();
    commandList->SetVertexBuffers(0, 1, &vertexBuffer);
    
    // 设置索引缓冲区
    commandList->SetIndexBuffer(m_fullscreenQuadIB.Get());
    
    // 绘制全屏四边形
    commandList->DrawIndexed(6, 1, 0, 0, 0);
    
    // 转换资源状态回渲染目标（以便下一帧重用）
    RALResourceBarrier finalBarrier = {};
    finalBarrier.type = RALResourceBarrierType::Transition;
    finalBarrier.resource = m_HDRSceneColor.Get();
    finalBarrier.oldState = RALResourceState::ShaderResource;
    finalBarrier.newState = RALResourceState::RenderTarget;
    commandList->ResourceBarriers(&finalBarrier, 1);
}

// 清理延迟着色相关资源
void Scene::Resize(uint32_t width, uint32_t height)
{
    if (!m_device)
    {
        logDebug("[DEBUG] Scene::Resize failed: device is null");
        return;
    }
    
    logDebug("[DEBUG] Scene::Resize called with width=" + std::to_string(width) + ", height=" + std::to_string(height));
    
    // 先释放现有渲染资源
    ReleaseRenderingResources();
    
    // 然后重新创建渲染资源
    if (!CreateRenderingResources(width, height))
    {
        logDebug("[DEBUG] Scene::Resize failed: CreateRenderingResources failed");
    }
}

void Scene::ReleaseRenderingResources()
{
    // 释放GBuffer资源
    m_gbufferA.Reset();
    m_gbufferARTV.Reset();
    m_gbufferASRV.Reset();
    
    m_gbufferB.Reset();
    m_gbufferBRTV.Reset();
    m_gbufferBSRV.Reset();
    
    m_gbufferC.Reset();
    m_gbufferCRTV.Reset();
    m_gbufferCSRV.Reset();
    
    // 释放深度模板缓冲区资源
    m_gbufferDepthStencil.Reset();
    m_gbufferDSV.Reset();
    m_gbufferDepthSRV.Reset();
    
    // 释放光照结果资源
    m_diffuseLightRT.Reset();
    m_diffuseLightRTV.Reset();
    m_diffuseLightSRV.Reset();
    
    m_specularLightRT.Reset();
    m_specularLightRTV.Reset();
    m_specularLightSRV.Reset();
    
    // 释放HDR场景颜色资源
    m_HDRSceneColor.Reset();
    m_HDRSceneColorRTV.Reset();
    m_HDRSceneColorSRV.Reset();
}

void Scene::CleanupDeferredRendering()
{
    // 释放渲染资源
    ReleaseRenderingResources();
    
    // 清理色调映射资源
    m_tonemappingRootSignature = nullptr;
    m_tonemappingPipelineState = nullptr;
    m_tonemappingVS = nullptr;
    m_tonemappingPS = nullptr;
    
    // 清理着色器和管道状态
    m_gbufferVertexShader = nullptr;
    m_gbufferPixelShader = nullptr;
    m_gbufferRootSignature = nullptr;
    m_gbufferPipelineState = nullptr;
    
    m_lightVertexShader = nullptr;
    m_lightPixelShader = nullptr;
    m_lightRootSignature = nullptr;
    m_lightPipelineState = nullptr;
    
    // 清理Resolve阶段相关资源
    m_resolvePipelineState = nullptr;
    m_resolveVertexShader = nullptr;
    m_resolvePixelShader = nullptr;
    m_resolveRootSignature = nullptr;
    
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

    // 创建顶点缓冲区，直接传递初始数据
    m_fullscreenQuadVB = m_device->CreateVertexBuffer(sizeof(vertices), sizeof(FullscreenQuadVertex), true, vertices, L"FullScreenQuadVB");

    // 定义索引数据
    uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    // 创建索引缓冲区，直接传递初始数据
    m_fullscreenQuadIB = m_device->CreateIndexBuffer(6, true, true, indices, L"FullScreenQuadIB");
}

// 执行几何阶段
void Scene::ExecuteGeometryPass(const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix)
{
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();

    // 在设置渲染目标之前，先将GBuffer和深度模板缓冲区转换为正确的渲染状态
    RALResourceBarrier barriers[4];
    
    // GBuffer A转换为渲染目标状态
    barriers[0].type = RALResourceBarrierType::Transition;
    barriers[0].resource = m_gbufferA.Get();
    barriers[0].oldState = RALResourceState::ShaderResource;  // 假设之前是ShaderResource状态
    barriers[0].newState = RALResourceState::RenderTarget;
    
    // GBuffer B转换为渲染目标状态
    barriers[1].type = RALResourceBarrierType::Transition;
    barriers[1].resource = m_gbufferB.Get();
    barriers[1].oldState = RALResourceState::ShaderResource;
    barriers[1].newState = RALResourceState::RenderTarget;
    
    // GBuffer C转换为渲染目标状态
    barriers[2].type = RALResourceBarrierType::Transition;
    barriers[2].resource = m_gbufferC.Get();
    barriers[2].oldState = RALResourceState::ShaderResource;
    barriers[2].newState = RALResourceState::RenderTarget;
    
    // 深度模板缓冲区转换为深度模板状态
    barriers[3].type = RALResourceBarrierType::Transition;
    barriers[3].resource = m_gbufferDepthStencil.Get();
    barriers[3].oldState = RALResourceState::ShaderResource;
    barriers[3].newState = RALResourceState::DepthStencil;
    
    // 一次性提交所有资源屏障
    commandList->ResourceBarriers(barriers, 4);

    // 设置渲染目标视图和深度模板视图
    IRALRenderTargetView* renderTargetViews[3] = { m_gbufferARTV.Get(), m_gbufferBRTV.Get(), m_gbufferCRTV.Get() };
    commandList->SetRenderTargets(3, renderTargetViews, m_gbufferDSV.Get());

    // 清除渲染目标视图和深度模板视图
    // GBuffer A使用与资源创建时匹配的clear value [0.0f, 0.0f, 1.0f, 1.0f]（默认法线值）
    float clearColorA[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    commandList->ClearRenderTarget(m_gbufferARTV.Get(), clearColorA);
    
    // GBuffer B和C使用与资源创建时匹配的clear value [0.0f, 0.0f, 0.0f, 1.0f]
    float clearColorBC[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTarget(m_gbufferBRTV.Get(), clearColorBC);
    commandList->ClearRenderTarget(m_gbufferCRTV.Get(), clearColorBC);
    commandList->ClearDepthStencil(m_gbufferDSV.Get(), RALClearFlags::Depth | RALClearFlags::Stencil, 1.0f, 0);

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
    
    // 几何阶段完成后，将GBuffer和深度模板缓冲区转换为着色器资源状态，以便光照阶段读取
    // 使用ResourceBarrier替代专门的状态转换方法
    RALResourceBarrier endBarriers[4];
    
    // GBuffer A从渲染目标状态转换为着色器资源状态
    endBarriers[0].type = RALResourceBarrierType::Transition;
    endBarriers[0].resource = m_gbufferA.Get();
    endBarriers[0].oldState = RALResourceState::RenderTarget;
    endBarriers[0].newState = RALResourceState::ShaderResource;
    
    // GBuffer B从渲染目标状态转换为着色器资源状态
    endBarriers[1].type = RALResourceBarrierType::Transition;
    endBarriers[1].resource = m_gbufferB.Get();
    endBarriers[1].oldState = RALResourceState::RenderTarget;
    endBarriers[1].newState = RALResourceState::ShaderResource;
    
    // GBuffer C从渲染目标状态转换为着色器资源状态
    endBarriers[2].type = RALResourceBarrierType::Transition;
    endBarriers[2].resource = m_gbufferC.Get();
    endBarriers[2].oldState = RALResourceState::RenderTarget;
    endBarriers[2].newState = RALResourceState::ShaderResource;
    
    // 深度模板缓冲区从深度模板状态转换为着色器资源状态
    endBarriers[3].type = RALResourceBarrierType::Transition;
    endBarriers[3].resource = m_gbufferDepthStencil.Get();
    endBarriers[3].oldState = RALResourceState::DepthStencil;
    endBarriers[3].newState = RALResourceState::ShaderResource;
    
    // 一次性提交所有资源屏障
    commandList->ResourceBarriers(endBarriers, 4);
}

// 执行光照阶段
void Scene::ExecuteLightingPass()
{
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();

    // 设置渲染目标为两个光照结果RT
    IRALRenderTargetView* renderTargets[2] = { m_diffuseLightRTV.Get(), m_specularLightRTV.Get() };
    commandList->SetRenderTargets(2, renderTargets, nullptr); // nullptr参数类型为IRALDepthStencilView*

    // 使用场景常量缓冲区，其中已经包含了所有需要的光照信息和invViewProj矩阵
    // 不需要单独更新光照常量缓冲区

    // 设置光照阶段根签名和管线状态
    commandList->SetGraphicsRootSignature(m_lightRootSignature.Get());
    commandList->SetPipelineState(m_lightPipelineState.Get());

    // 设置根参数0（场景常量，包含invViewProj和光照信息）
    commandList->SetGraphicsRootConstantBuffer(0, m_sceneConstBuffer.Get());

    // 绑定GBuffer纹理到描述符表
    commandList->SetGraphicsRootDescriptorTable(1, m_gbufferASRV.Get());
    commandList->SetGraphicsRootDescriptorTable(2, m_gbufferBSRV.Get());
    commandList->SetGraphicsRootDescriptorTable(3, m_gbufferCSRV.Get());
    // 绑定深度纹理到描述符表（使用根参数4）
    commandList->SetGraphicsRootDescriptorTable(4, m_gbufferDepthSRV.Get());

    // 设置全屏四边形
    IRALVertexBuffer* vertexBuffer = m_fullscreenQuadVB.Get();
    commandList->SetVertexBuffers(0, 1, &vertexBuffer);
    commandList->SetIndexBuffer(m_fullscreenQuadIB.Get());
    commandList->SetPrimitiveTopology(RALPrimitiveTopologyType::TriangleList);

    // 绘制全屏四边形
    commandList->DrawIndexed(6, 1, 0, 0, 0);
}

// 执行GBuffer Resolve阶段
void Scene::ExecuteResolvePass()
{
    IRALGraphicsCommandList* commandList = m_device->GetGraphicsCommandList();
    
    // 资源屏障：确保HDR场景颜色RT处于正确的状态
    RALResourceBarrier barriers[5];
    
    // 将HDRSceneColor转换为渲染目标状态
    barriers[0].type = RALResourceBarrierType::Transition;
    barriers[0].resource = m_HDRSceneColor.Get();
    barriers[0].oldState = RALResourceState::ShaderResource;
    barriers[0].newState = RALResourceState::RenderTarget;
    
    // 确保所有输入资源处于着色器资源状态
    barriers[1].type = RALResourceBarrierType::Transition;
    barriers[1].resource = m_diffuseLightRT.Get();
    barriers[1].oldState = RALResourceState::RenderTarget;
    barriers[1].newState = RALResourceState::ShaderResource;
    
    barriers[2].type = RALResourceBarrierType::Transition;
    barriers[2].resource = m_specularLightRT.Get();
    barriers[2].oldState = RALResourceState::RenderTarget;
    barriers[2].newState = RALResourceState::ShaderResource;
    
    // 提交资源屏障
    commandList->ResourceBarriers(barriers, 3);
    
    // 设置渲染目标为HDR场景颜色
    IRALRenderTargetView* renderTargets[1] = { m_HDRSceneColorRTV.Get() };
    commandList->SetRenderTargets(1, renderTargets, nullptr);
    
    // 设置Resolve阶段根签名和管线状态
    commandList->SetGraphicsRootSignature(m_resolveRootSignature.Get());
    commandList->SetPipelineState(m_resolvePipelineState.Get());
    
    // 设置根参数0（场景常量缓冲区）
    commandList->SetGraphicsRootConstantBuffer(0, m_sceneConstBuffer.Get());
    
    // 绑定所有需要的纹理到描述符表
    commandList->SetGraphicsRootDescriptorTable(1, m_diffuseLightSRV.Get());
    commandList->SetGraphicsRootDescriptorTable(2, m_specularLightSRV.Get());
    commandList->SetGraphicsRootDescriptorTable(3, m_gbufferBSRV.Get());
    commandList->SetGraphicsRootDescriptorTable(4, m_gbufferCSRV.Get());
    
    // 设置全屏四边形
    IRALVertexBuffer* vertexBuffer = m_fullscreenQuadVB.Get();
    commandList->SetVertexBuffers(0, 1, &vertexBuffer);
    commandList->SetIndexBuffer(m_fullscreenQuadIB.Get());
    commandList->SetPrimitiveTopology(RALPrimitiveTopologyType::TriangleList);
    
    // 绘制全屏四边形
    commandList->DrawIndexed(6, 1, 0, 0, 0);
    
    // 渲染完成后，将HDRSceneColor转换回ShaderResource状态，以便后续使用
    RALResourceBarrier finalBarrier;
    finalBarrier.type = RALResourceBarrierType::Transition;
    finalBarrier.resource = m_HDRSceneColor.Get();
    finalBarrier.oldState = RALResourceState::RenderTarget;
    finalBarrier.newState = RALResourceState::ShaderResource;
    
    commandList->ResourceBarriers(&finalBarrier, 1);
}