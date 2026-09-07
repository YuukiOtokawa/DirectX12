#pragma once

#include "../Manager/Main.h"
#include "RenderTargetType.h"
#include <memory>

namespace EngineCore::Render {

	class DescriptorAllocator;
	namespace Types { struct RENDER_TARGET; }

	class RenderTargetManager
	{
	public:
		// シャドウカスケードアトラス（2x2グリッド、1枠=TILEサイズの正方形）
		static const unsigned int SHADOW_ATLAS_SIZE = 2048;
		static const unsigned int SHADOW_CASCADE_TILE = 1024;

		// ブルームのミップピラミッド段数の上限（実際の段数は解像度から決まる）
		static const unsigned int BLOOM_MAX_MIPS = 5;

		~RenderTargetManager();

		void InitBackBufferAndDepth(ID3D12Device* device, IDXGISwapChain3* swapChain, int backBufferWidth, int backBufferHeight);
		void InitGBuffers(ID3D12Device* device, DescriptorAllocator& srvAllocator, DescriptorAllocator& rtvAllocator);

		void CreateBackBufferTargets(ID3D12Device* device, IDXGISwapChain3* swapChain);
		void ResetBackBuffer();

		ID3D12Resource* GetBackBufferResource(unsigned int index) const { return m_BackBuffer[index].Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferHandle(unsigned int index) const { return m_BackBufferHandle[index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferHandle() const { return m_DepthBufferHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetShadowDepthBufferHandle() const { return m_ShadowDepthBufferHandle; }
		const D3D12_VIEWPORT& GetViewport() const { return m_Viewport; }
		const D3D12_RECT& GetScissorRect() const { return m_ScissorRect; }

		Types::RENDER_TARGET* GetColorBuffer() const { return m_ColorBuffer.get(); }
		Types::RENDER_TARGET* GetNormalBuffer() const { return m_NormalBuffer.get(); }
		Types::RENDER_TARGET* GetPositionBuffer() const { return m_PositionBuffer.get(); }
		Types::RENDER_TARGET* GetMaterialBuffer() const { return m_MaterialBuffer.get(); }
		Types::RENDER_TARGET* GetEmissionBuffer() const { return m_EmissionBuffer.get(); }
		Types::RENDER_TARGET* GetPostProcessBuffer() const { return m_PostProcessBuffer1.get(); }
        Types::RENDER_TARGET *GetShadowMapBuffer() const { return m_ShadowMapBuffer.get(); }
		Types::RENDER_TARGET* GetLightedColorBuffer() const { return m_LightedColorBuffer.get(); }
		Types::RENDER_TARGET* GetBloomMipUp(unsigned int index) const { return m_BloomMipUp[index].get(); }
		Types::RENDER_TARGET* GetBloomMipDown(unsigned int index) const { return m_BloomMipDown[index].get(); }
		unsigned int GetBloomMipCount() const { return m_BloomMipCount; }
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
		// 深度リソースを生成して指定のDSVハンドルへビューを作る（メイン／シャドウ共用）
		void CreateDepthResource(ID3D12Device* device, unsigned int width, unsigned int height, ComPtr<ID3D12Resource>& outBuffer, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
		void CreateDepthBuffer(ID3D12Device* device, unsigned int width, unsigned int height);
		void CreateShadowDepthBuffer(ID3D12Device* device);

		ComPtr<ID3D12Resource>				m_BackBuffer[2];
		ComPtr<ID3D12DescriptorHeap>		m_BackBufferDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_BackBufferHandle[2];

		ComPtr<ID3D12Resource>				m_DepthBuffer;
		ComPtr<ID3D12DescriptorHeap>		m_DepthBufferDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_DepthBufferHandle;

		// シャドウアトラス専用の深度バッファ（2048x2048、リサイズ対象外）
		ComPtr<ID3D12Resource>				m_ShadowDepthBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_ShadowDepthBufferHandle{};

		D3D12_VIEWPORT						m_Viewport;
		D3D12_RECT							m_ScissorRect;

		std::unique_ptr<Types::RENDER_TARGET>	m_ColorBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_NormalBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_PositionBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_MaterialBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_EmissionBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_LightedColorBuffer;
		std::unique_ptr<Types::RENDER_TARGET>	m_PostProcessBuffer1;
        std::unique_ptr<Types::RENDER_TARGET>	m_ShadowMapBuffer;
		// ブルーム用ミップピラミッド。ブラーが H/V で ping-pong するため 2 本必要
		// （BlurH: MipDown[i-1] -> MipUp[i] / BlurV: MipUp[i] -> MipDown[i]）
		std::unique_ptr<Types::RENDER_TARGET>	m_BloomMipUp[BLOOM_MAX_MIPS];
		std::unique_ptr<Types::RENDER_TARGET>	m_BloomMipDown[BLOOM_MAX_MIPS];
		unsigned int							m_BloomMipCount = 0;

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
