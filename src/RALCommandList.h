#ifndef RALCOMMANDLIST_H
#define RALCOMMANDLIST_H

#include <cstdint>
#include <vector>

// 命令列表类型（区分图形和计算）
enum class RALCommandListType 
{
	Graphics,  // 图形命令列表（渲染管线）
	Compute    // 计算命令列表（计算管线）
};

// 资源屏障类型（控制资源状态转换）
enum class RALResourceState {
	// 通用状态
	Undefined,
	Common,
	// 纹理状态
	ShaderResource,
	RenderTarget,
	DepthWrite,
	DepthRead,
	// 缓冲区状态
	ConstantBuffer,
	VertexBuffer,
	IndexBuffer,
	UnorderedAccess,
	// 传输状态（CPU-GPU数据传输）
	CopySource,
	CopyDestination
};

// 资源屏障（用于资源状态转换）
struct RALResourceBarrier {
	void* resource;       // 目标资源指针
	RALResourceState before;          // 转换前状态
	RALResourceState after;           // 转换后状态
	uint32_t subresource = 0;      // 子资源索引（默认0表示整个资源）
};

// 命令列表抽象基类
class IRALCommandList {
public:
	IRALCommandList(RALCommandListType type) : m_type(type) {}
	virtual ~IRALCommandList() = default;

	// 重置命令列表（重用前必须调用）
	virtual void reset() = 0;

	// 关闭命令列表（记录完成，准备提交）
	virtual void close() = 0;

	// 资源状态转换（跨平台统一接口）
	virtual void resourceBarrier(const std::vector<RALResourceBarrier>& barriers) = 0;

	// 获取命令列表类型
	RALCommandListType getType() const { return m_type; }

protected:
	RALCommandListType m_type;
};

// 图形命令列表（继承自CommandList，增加图形渲染相关接口）
class IRALGraphicsCommandList : public IRALCommandList {
public:
	IRALGraphicsCommandList() : IRALCommandList(RALCommandListType::Graphics) {}

	// 绑定图形管线
	virtual void bindPipeline(void* pipeline) = 0;

	// 设置视口（可多个，索引0为默认）
	virtual void setViewports(const std::vector<void*>& viewports) = 0;

	// 设置裁剪矩形（可多个，索引0为默认）
	virtual void setScissorRects(const std::vector<void*>& scissors) = 0;

	// 绑定顶点缓冲区
	virtual void bindVertexBuffers(uint32_t slot, const std::vector<void*>& buffers) = 0;

	// 绑定索引缓冲区
	virtual void bindIndexBuffer(void* buffer, bool is32Bit) = 0;

	// 绑定渲染目标和深度模板
	virtual void bindRenderTargets(
		const std::vector<void*>& renderTargets,
		void* depthStencil = nullptr
	) = 0;

	// 绘制（无索引）
	virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, 
		uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;

	// 绘制（有索引）
	virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
		uint32_t firstIndex = 0, int32_t vertexOffset = 0,
		uint32_t firstInstance = 0) = 0;

};

#endif // RALCOMMANDLIST_H