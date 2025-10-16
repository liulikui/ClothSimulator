#include "DX12RALResource.h"
#include "DX12RALDevice.h"
#include "RALCommandList.h"
#include "DX12RALCommandList.h"
#include <d3d12.h>
#include <iostream>

// DX12RALRenderTargetView析构函数实现
DX12RALRenderTargetView::~DX12RALRenderTargetView()
{
	if (m_device && m_rtvHandle.ptr != 0)
	{
		m_device->ReleaseRTVDescriptor(m_rtvHandle, m_rtvIndex, m_rtvHeap.Get());
	}
}

// DX12RALDepthStencilView析构函数实现
DX12RALDepthStencilView::~DX12RALDepthStencilView()
{
	if (m_device && m_dsvHandle.ptr != 0)
	{
		m_device->ReleaseDSVDescriptor(m_dsvHandle, m_dsvIndex, m_dsvHeap.Get());
	}
}

// DX12RALShaderResourceView析构函数实现
DX12RALShaderResourceView::~DX12RALShaderResourceView()
{
	if (m_device && m_srvHandle.ptr != 0)
	{
		m_device->ReleaseSRVDescriptor(m_srvHandle, m_srvIndex, m_srvHeap.Get());
	}
}
