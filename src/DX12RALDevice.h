#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "Camera.h"
#include "RALResource.h"
#include "RALCommandList.h"
#include "IRALDevice.h"
#include "DX12RALResource.h"
#include "TRefCountPtr.h"

// 简化命名空间
namespace dx = DirectX;

namespace std 
{
    template<> struct hash<ComPtr<ID3D12Resource>> 
    {
        size_t operator()(const ComPtr<ID3D12Resource>& u) const 
        {
			size_t hash_vallue = u.Get() ? std::hash<void*>()(u.Get()) : 0;
            return hash_vallue;
        }
    };
}

template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType, size_t size>
class DX12DescriptorHeapManager
{
public:
    DX12DescriptorHeapManager()
        : m_descriptorSize(0)
        , m_heapInfoHead(nullptr)
        , m_totalCount(0)
    {

    }

    ~DX12DescriptorHeapManager()
    {
        HeapInfo* heapinfo = m_heapInfoHead;

        while (heapinfo != nullptr)
        {
            HeapInfo* nextHeapinfo = heapinfo->next;
            delete heapinfo;
            heapinfo = nextHeapinfo;
        }
    }

    void SetDevice(const ComPtr<ID3D12Device>& device)
    {
        m_device = device;
    }

    void SetDescriptorSize(uint32_t size)
    {
        m_descriptorSize = size;
    }

    uint32_t GetDescriptorSize() const
    {
        return m_descriptorSize;
    }

    bool AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGPUHandle, ComPtr<ID3D12DescriptorHeap>& outHeap, uint32_t& outIndex)
    {
        if (GetAvailableDescriptor(outCPUHandle, outGPUHandle, outHeap, outIndex))
        {
            m_totalCount++;
            return true;
        }

        ComPtr<ID3D12DescriptorHeap> newHeap;
        if (CreateDescriptorHeap(newHeap) && newHeap != nullptr)
        {
            HeapInfo* newHeapInfo = new HeapInfo;
            newHeapInfo->heap = newHeap;
            newHeapInfo->size = size;
            newHeapInfo->next = m_heapInfoHead;
            
            if (m_heapInfoHead != nullptr)
            {
                m_heapInfoHead->prev = newHeapInfo;
            }

            m_heapInfoHead = newHeapInfo;

            outHeap = newHeap;
            outIndex = newHeapInfo->curIndex++;
            
            // 设置CPU描述符句柄
            outCPUHandle = outHeap->GetCPUDescriptorHandleForHeapStart();
            outCPUHandle.ptr += outIndex * m_descriptorSize;
            
            if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
            {
                // 设置GPU描述符句柄
                outGPUHandle = outHeap->GetGPUDescriptorHandleForHeapStart();
                outGPUHandle.ptr += outIndex * m_descriptorSize;
            }

            m_totalCount++;

            return true;
        }

        return false;
    }

    bool FreeDescriptor(ID3D12DescriptorHeap* heap, uint32_t index)
    {
        HeapInfo* heapinfo = m_heapInfoHead;

        while (heapinfo != nullptr)
        {
            if (heapinfo->heap.Get() == heap)
            {
                bool found = false;
                for (size_t i = 0; i < heapinfo->freeSlots.size(); ++i)
                {
                    if (heapinfo->freeSlots[i] == index)
                    {
                        found = true;
                        break;
                    }
                }

                if (found)
                {
                    return false;
                }
                else
                {
                    heapinfo->freeSlots.push_back(index);

                    return true;
                }
            }

            heapinfo = heapinfo->next;
        }

        return false;
    }

private:
    bool CreateDescriptorHeap(ComPtr<ID3D12DescriptorHeap>& outHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = HeapType;
        heapDesc.NumDescriptors = size;

        heapDesc.Flags = (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? 
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(outHeap.ReleaseAndGetAddressOf()));

        return SUCCEEDED(hr);
    }

    bool GetAvailableDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGPUHandle, ComPtr<ID3D12DescriptorHeap>& outHeap, uint32_t& outIndex)
    {
        HeapInfo* heapinfo = m_heapInfoHead;

        while (heapinfo != nullptr)
        {
            if (heapinfo->freeSlots.size() > 0)
            {
                outHeap = heapinfo->heap;
                outIndex = heapinfo->freeSlots.back();
                heapinfo->freeSlots.pop_back();
                
                // 设置CPU描述符句柄
                outCPUHandle = outHeap->GetCPUDescriptorHandleForHeapStart();
                outCPUHandle.ptr += outIndex * m_descriptorSize;
                
                if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                {
                    // 设置GPU描述符句柄
                    outGPUHandle = outHeap->GetGPUDescriptorHandleForHeapStart();
					outGPUHandle.ptr += outIndex * m_descriptorSize;
                }

                return true;
            }
            else if (heapinfo->curIndex + 1 < heapinfo->size)
            {
                outHeap = heapinfo->heap;
                outIndex = heapinfo->curIndex++;
                
                // 设置CPU描述符句柄
                outCPUHandle = outHeap->GetCPUDescriptorHandleForHeapStart();
                outCPUHandle.ptr += outIndex * m_descriptorSize;
                
                if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                {
                    // 设置GPU描述符句柄
                    outGPUHandle = outHeap->GetGPUDescriptorHandleForHeapStart();
                    outGPUHandle.ptr += outIndex * m_descriptorSize;
                }

                return true;
            }

            heapinfo = heapinfo->next;
        }

        return false;
    }

private:
    struct HeapInfo
    {
        HeapInfo()
            : size(0)
            , curIndex(0)
            , next(nullptr)
            , prev(nullptr)
        {
        }

        ComPtr<ID3D12DescriptorHeap> heap;
        uint32_t size;
        uint32_t curIndex;
        std::vector<uint32_t> freeSlots;
        HeapInfo* next;
        HeapInfo* prev;
    };

    ComPtr<ID3D12Device> m_device;
    uint32_t m_descriptorSize;
    HeapInfo* m_heapInfoHead;
    size_t m_totalCount;
};

class DX12RALDevice : public IRALDevice
{
public:
    // 构造函数和析构函数
    DX12RALDevice(uint32_t width, uint32_t height, const std::wstring& windowName, HWND hWnd);
    ~DX12RALDevice();

    // 初始化DirectX 12
    virtual bool Initialize() override;

    // 开始一帧
    virtual void BeginFrame() override;

    //// 提交CommandLists
    //void ExecuteCommandLists(uint32_t count, IRALCommandList** ppCommandList);

    // 结束一帧
    virtual void EndFrame() override;

    // 清理资源
    virtual void Cleanup() override;

    // 调整窗口大小
    virtual void Resize(uint32_t width, uint32_t height) override;

    // 获取窗口宽度
    virtual uint32_t GetWidth() const override { return m_width; }

    // 获取窗口高度
    virtual uint32_t GetHeight() const override { return m_height; }
    
    // 编译顶点着色器
    virtual IRALVertexShader* CompileVertexShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译像素着色器
    virtual IRALPixelShader* CompilePixelShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译几何着色器
    virtual IRALGeometryShader* CompileGeometryShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译计算着色器
    virtual IRALComputeShader* CompileComputeShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译网格着色器
    virtual IRALMeshShader* CompileMeshShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译放大着色器
    virtual IRALAmplificationShader* CompileAmplificationShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译光线生成着色器
    virtual IRALRayGenShader* CompileRayGenShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译光线未命中着色器
    virtual IRALRayMissShader* CompileRayMissShader(const char* shaderCode, const char* entryPoint = "main") override;
    
    // 编译光线命中组着色器
	virtual IRALRayHitGroupShader* CompileRayHitGroupShader(const char* shaderCode, const char* entryPoint = "main") override;
	
	// 编译光线可调用着色器
	virtual IRALRayCallableShader* CompileRayCallableShader(const char* shaderCode, const char* entryPoint = "main") override;
	
	// 创建图形管线状态
    virtual IRALGraphicsPipelineState* CreateGraphicsPipelineState(const RALGraphicsPipelineStateDesc& desc, const wchar_t* debugName = nullptr) override;

    // 创建根签名 
    virtual IRALRootSignature* CreateRootSignature(const std::vector<RALRootParameter>& rootParameters,
        const std::vector<RALStaticSampler>& staticSamplers = {},
        RALRootSignatureFlags flags = RALRootSignatureFlags::AllowInputAssemblerInputLayout, const wchar_t* debugName = nullptr) override;
        
    //// 创建图形命令列表
    //IRALGraphicsCommandList* CreateGraphicsCommandList();

    // 创建顶点缓冲区
    virtual IRALVertexBuffer* CreateVertexBuffer(uint32_t size, uint32_t stride, bool isStatic, const void* initialData = nullptr, const wchar_t* debugName = nullptr) override;

    // 创建索引缓冲区
    virtual IRALIndexBuffer* CreateIndexBuffer(uint32_t count, bool is32BitIndex, bool isStatic, const void* initialData = nullptr, const wchar_t* debugName = nullptr) override;

    // 创建常量缓冲区
    virtual IRALConstBuffer* CreateConstBuffer(uint32_t size, const wchar_t* debugName = nullptr) override;

    // 更新Buffer
    virtual bool UploadBuffer(IRALBuffer* buffer, const char* data, uint64_t size) override;

    // 获得GraphicsCommandList
    virtual IRALGraphicsCommandList* GetGraphicsCommandList() override;

    // 创建渲染目标
    virtual IRALRenderTarget* CreateRenderTarget(uint32_t width, uint32_t height, RALDataFormat format, const RALClearValue* clearValue = nullptr, const wchar_t* debugName = nullptr) override;

    // 创建深度/模板缓冲区
    virtual IRALDepthStencil* CreateDepthStencil(uint32_t width, uint32_t height, RALDataFormat format, const RALClearValue* clearValue = nullptr, const wchar_t* debugName = nullptr) override;
    
    // 创建渲染目标视图
    virtual IRALRenderTargetView* CreateRenderTargetView(IRALRenderTarget* renderTarget, const RALRenderTargetViewDesc& desc, const wchar_t* debugName = nullptr) override;

    // 创建深度模板视图
    virtual IRALDepthStencilView* CreateDepthStencilView(IRALDepthStencil* depthStencil, const RALDepthStencilViewDesc& desc, const wchar_t* debugName = nullptr) override;

    // 创建着色器资源视图
    virtual IRALShaderResourceView* CreateShaderResourceView(IRALResource* resource, const RALShaderResourceViewDesc& desc, const wchar_t* debugName = nullptr) override;

    // 释放渲染目标视图描述符
    bool ReleaseRTVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t index, ID3D12DescriptorHeap* heap);

    // 释放深度模板视图描述符
    bool ReleaseDSVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t index, ID3D12DescriptorHeap* heap);

    // 释放着色器资源视图描述符
    bool ReleaseSRVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t index, ID3D12DescriptorHeap* heap);
    
    // 获取backbuffer的渲染目标视图
    virtual IRALRenderTargetView* GetBackBufferRTV() override;
    
    // 获取backbuffer的深度模板视图
    virtual IRALDepthStencilView* GetBackBufferDSV() override;

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
    void CreateMainRenderTargetViews();

    // 创建深度/模板视图
    void CreateMainDepthStencilView();

    // 创建缓冲区
    ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
        D3D12_HEAP_PROPERTIES heapProps,
        D3D12_RESOURCE_STATES initialState);

    // 等待操作完成
    void WaitForPreviousOperations();

    // 等待前一帧完成
    void WaitForPreviousFrame();

	// 检查资源是否正在上传
	bool IsUploadingResource(ID3D12Resource* resource) const;
	void AddUploadingResource(ID3D12Resource* resource, const ComPtr<ID3D12Resource>& uploadBuffer);

private:
    // 成员变量
    uint32_t m_width;                                           // 窗口宽度
    uint32_t m_height;                                          // 窗口高度
    std::wstring m_windowName;                                  // 窗口名称
    HWND m_hWnd;                                                // 窗口句柄

    // 设备和交换链
    ComPtr<ID3D12Device> m_device;                              // D3D12设备
    
    ComPtr<IDXGIFactory6> m_factory;                            // DXGI工厂
    ComPtr<IDXGISwapChain4> m_swapChain;                        // 交换链
    uint32_t m_backBufferCount = 2;                             // 后缓冲区数量
    uint32_t m_currentBackBufferIndex = 0;                      // 当前后缓冲区索引

    // 命令对象 - 主渲染
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[2];      // 命令分配器数组
    ComPtr<ID3D12CommandQueue> m_commandQueue;                  // 命令队列
    TRefCountPtr<IRALGraphicsCommandList> m_graphicsCommandList;   // 渲染命令列表

    // 同步对象 - 主渲染
    ComPtr<ID3D12Fence> m_fence;                                // 围栏
    uint64_t m_fenceValue = 0;                                  // 围栏值
    HANDLE m_fenceEvent = nullptr;                              // 围栏事件

    uint32_t m_currentFrameIndex;                               // 当前帧索引，用于缓存

    // 描述符堆和管理
    // 主描述符堆（用于后缓冲区和主深度缓冲区）
    ComPtr<ID3D12DescriptorHeap> m_mainRtvHeap;                 // 主渲染目标视图堆
    ComPtr<ID3D12DescriptorHeap> m_mainDsvHeap;                 // 主深度/模板视图堆
    ComPtr<ID3D12DescriptorHeap> m_mainSrvHeap;                 // 主着色器资源视图堆
    
    // 描述符大小
    uint32_t m_rtvDescriptorSize = 0;                           // 渲染目标视图描述符大小
    uint32_t m_dsvDescriptorSize = 0;                           // 深度/模板视图描述符大小
    uint32_t m_srvDescriptorSize = 0;                           // 着色器资源视图描述符大小
    
    DX12DescriptorHeapManager<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 32> m_RTVDescriptorHeaps;
    DX12DescriptorHeapManager<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32> m_DSVDescriptorHeaps;
    DX12DescriptorHeapManager<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32> m_SRVDescriptorHeaps;

    // 资源
    std::vector<ComPtr<ID3D12Resource>> m_backBuffers;          // 后缓冲区
    ComPtr<ID3D12Resource> m_depthStencilBuffer;                // 深度/模板缓冲区
    
    // Backbuffer的RTV和DSV
    std::vector<TRefCountPtr<IRALRenderTargetView>> m_backBufferRTVs;      // 后缓冲区渲染目标视图
    TRefCountPtr<IRALDepthStencilView> m_mainDepthStencilView;              // 主深度模板视图

    struct UploadingResourceInfo
    {
        std::vector<ComPtr<ID3D12Resource>> uploadBuffers;
	};

    typedef std::unordered_map<ComPtr<ID3D12Resource>, UploadingResourceInfo> UploadingResources;
    UploadingResources m_uploadingResources;
};
