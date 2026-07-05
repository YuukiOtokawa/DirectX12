#pragma once

#include "Main.h"
#include "RenderTargetType.h"
#include <memory>

namespace EngineCore::Render {

	class DescriptorAllocator;
	namespace Types { struct RENDER_TARGET; }

	class RenderTargetManager
	{
	public:
		~RenderTargetManager();

		void InitBackBufferAndDepth(ID3D12Device* device, IDXGISwapChain3* swapChain, int backBufferWidth, int backBufferHeight);
		void InitGBuffers(ID3D12Device* device, DescriptorAllocator& srvAllocator, DescriptorAllocator& rtvAllocator);

		void CreateBackBufferTargets(ID3D12Device* device, IDXGISwapChain3* swapChain);
		void ResetBackBuffer();

		ID3D12Resource* GetBackBufferResource(unsigned int index) const { return m_BackBuffer[index].Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferHandle(unsigned int index) const { return m_BackBufferHandle[index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferHandle() const { return m_DepthBufferHandle; }
		const D3D12_VIEWPORT& GetViewport() const { return m_Viewport; }
		const D3D12_RECT& GetScissorRect() const { return m_ScissorRect; }

		Types::RENDER_TARGET* GetColorBuffer() const { return m_ColorBuffer.get(); }
		Types::RENDER_TARGET* GetNormalBuffer() const { return m_NormalBuffer.get(); }
		Types::RENDER_TARGET* GetPositionBuffer() const { return m_PositionBuffer.get(); }
		Types::RENDER_TARGET* GetMaterialBuffer() const { return m_MaterialBuffer.get(); }
		Types::RENDER_TARGET* GetEmissionBuffer() const { return m_EmissionBuffer.get(); }
		Types::RENDER_TARGET* GetPostProcessBuffer() const { return m_PostProcessBuffer1.get(); }
		Types::RENDER_TARGET* GetLightedColorBuffer() const { return m_LightedColorBuffer.get(); }
		Types::RENDER_TARGET* GetGameViewTarget() const { return m_GameViewTarget.get(); }
		Types::RENDER_TARGET* GetSceneViewTarget() const { return m_SceneViewTarget.get(); }

		void SetCurrentTarget(RenderTargetType targetType) { m_CurrentTargetType = targetType; }
		RenderTargetType GetCurrentTarget() const { return m_CurrentTargetType; }

		void GetActiveTargetSize(unsigned int& width, unsigned int& height, int backBufferWidth, int backBufferHeight) const;

		void Resize(unsigned int width, unsigned int height);
		void ResizeTarget(RenderTargetType type, unsigned int width, unsigned int height);
		bool HasPendingResize() const { return m_SwapChainResizePending || m_GameViewResizePending || m_SceneViewResizePending; }
		bool HasPendingSwapChainResize() const { return m_SwapChainResizePending; }

		void ApplyPendingResizes(ID3D12Device* device, IDXGISwapChain3* swapChain, DescriptorAllocator& srvAllocator, DescriptorAllocator& rtvAllocator, int& backBufferWidth, int& backBufferHeight);

	private:
		void CreateDepthBuffer(ID3D12Device* device, unsigned int width, unsigned int height);

		ComPtr<ID3D12Resource>				m_BackBuffer[2];
		ComPtr<ID3D12DescriptorHeap>		m_BackBufferDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_BackBufferHandle[2];

		ComPtr<ID3D12Resource>				m_DepthBuffer;
		ComPtr<ID3D12DescriptorHeap>		m_DepthBufferDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_DepthBufferHandle;

		D3D12_VIEWPORT						m_Viewport;
		D3D12_RECT							m_ScissorRect;

		std::unique_ptr<Types::RENDER_TARGET>	m_ColorBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_NormalBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_PositionBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_MaterialBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_EmissionBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_LightedColorBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_PostProcessBuffer1;

		std::unique_ptr<Types::RENDER_TARGET>	m_GameViewTarget;
		std::unique_ptr<Types::RENDER_TARGET>	m_SceneViewTarget;

		RenderTargetType m_CurrentTargetType = RenderTargetType::GAME_VIEW;

		bool m_SwapChainResizePending = false;
		unsigned int m_SwapChainPendingWidth = 0;
		unsigned int m_SwapChainPendingHeight = 0;

		bool m_GameViewResizePending = false;
		unsigned int m_GameViewPendingWidth = 0;
		unsigned int m_GameViewPendingHeight = 0;

		bool m_SceneViewResizePending = false;
		unsigned int m_SceneViewPendingWidth = 0;
		unsigned int m_SceneViewPendingHeight = 0;
	};

}
