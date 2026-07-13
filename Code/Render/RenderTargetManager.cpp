#include "../Manager/Main.h"
#include "RenderTargetManager.h"
#include "RenderManager.h"
#include "RenderTargetFactory.h"
#include "D3DX12.h"
#include <cassert>

namespace EngineCore::Render {

	RenderTargetManager::~RenderTargetManager() = default;

	void RenderTargetManager::InitBackBufferAndDepth(ID3D12Device* device, IDXGISwapChain3* swapChain, int backBufferWidth, int backBufferHeight)
	{
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;
		m_Viewport.Width = (FLOAT)backBufferWidth;
		m_Viewport.Height = (FLOAT)backBufferHeight;
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;

		m_ScissorRect.top = 0;
		m_ScissorRect.left = 0;
		m_ScissorRect.right = backBufferWidth;
		m_ScissorRect.bottom = backBufferHeight;

		// RenderTargetDescriptorHeap
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
			heapDesc.NumDescriptors = 2;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NodeMask = 0;

			HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_BackBufferDescriptorHeap));
			assert(SUCCEEDED(hr));

			UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			for (UINT i = 0; i < 2; ++i)
			{
				m_BackBufferHandle[i] = m_BackBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				m_BackBufferHandle[i].ptr += size * i;
			}
		}

		CreateBackBufferTargets(device, swapChain);
		m_BackBuffer[0]->SetName(L"RenderTarget");
		m_BackBuffer[1]->SetName(L"RenderTarget");

		// DepthBufferDescriptorHeap（0: メイン深度, 1: シャドウアトラス深度）
		{
			D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
			descriptorHeapDesc.NumDescriptors = 2;
			descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			descriptorHeapDesc.NodeMask = 0;

			HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DepthBufferDescriptorHeap));
			assert(SUCCEEDED(hr));
		}

		CreateDepthBuffer(device, backBufferWidth, backBufferHeight);
		m_DepthBuffer->SetName(L"DepthBuffer");

		CreateShadowDepthBuffer(device);
		m_ShadowDepthBuffer->SetName(L"ShadowDepthBuffer");
	}

	void RenderTargetManager::InitGBuffers(ID3D12Device* device, DescriptorAllocator& srvAllocator, DescriptorAllocator& rtvAllocator)
	{
		FLOAT colorClear[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		FLOAT zeroClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_ColorBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, colorClear);
		m_ColorBuffer->Resource->SetName(L"ColorBuffer");

		m_NormalBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_NormalBuffer->Resource->SetName(L"NormalBuffer");

		m_PositionBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_PositionBuffer->Resource->SetName(L"PositionBuffer");

		m_MaterialBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_MaterialBuffer->Resource->SetName(L"MaterialBuffer");

		m_EmissionBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_EmissionBuffer->Resource->SetName(L"EmissionBuffer");

		m_LightedColorBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_LightedColorBuffer->Resource->SetName(L"LightedColorBuffer");

        FLOAT clearColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
		// カスケードアトラス（2x2グリッドに最大4カスケードを敷き詰める）
		m_ShadowMapBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE, DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor);
		m_ShadowMapBuffer->Resource->SetName(L"ShadowMapBuffer");

		m_PostProcessBuffer1 = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_PostProcessBuffer1->Resource->SetName(L"PostProcessBuffer1");
	}

	void RenderTargetManager::CreateBackBufferTargets(ID3D12Device* device, IDXGISwapChain3* swapChain)
	{
		for (UINT i = 0; i < 2; i++) {
			ComPtr<ID3D12Resource> backBuffer;
			HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
			assert(SUCCEEDED(hr));

			device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_BackBufferHandle[i]);
			m_BackBuffer[i] = backBuffer;
		}
	}

	void RenderTargetManager::CreateDepthResource(ID3D12Device* device, unsigned int width, unsigned int height, ComPtr<ID3D12Resource>& outBuffer, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = width;
		resourceDesc.Height = height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		HRESULT hr = device->CreateCommittedResource(&prop,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&outBuffer));
		assert(SUCCEEDED(hr));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(outBuffer.Get(), &dsvDesc, dsvHandle);
	}

	void RenderTargetManager::CreateDepthBuffer(ID3D12Device* device, unsigned int width, unsigned int height)
	{
		m_DepthBufferHandle = m_DepthBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		CreateDepthResource(device, width, height, m_DepthBuffer, m_DepthBufferHandle);
	}

	void RenderTargetManager::CreateShadowDepthBuffer(ID3D12Device* device)
	{
		unsigned int increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_ShadowDepthBufferHandle = m_DepthBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_ShadowDepthBufferHandle.ptr += increment; // ヒープ1番: シャドウアトラス深度
		CreateDepthResource(device, SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE, m_ShadowDepthBuffer, m_ShadowDepthBufferHandle);
	}

	void RenderTargetManager::ResetBackBuffer()
	{
		if (m_BackBuffer[0]) {
			m_BackBuffer[0].Reset();
		}
		if (m_BackBuffer[1]) {
			m_BackBuffer[1].Reset();
		}
	}

	void RenderTargetManager::GetActiveTargetSize(unsigned int& width, unsigned int& height, int backBufferWidth, int backBufferHeight) const
	{
		if (m_CurrentTargetType == RenderTargetType::BACK_BUFFER) {
			width = backBufferWidth;
			height = backBufferHeight;
		}
		else {
			Types::RENDER_TARGET* target = (m_CurrentTargetType == RenderTargetType::GAME_VIEW) ? m_GameViewTarget.get() : m_SceneViewTarget.get();
			if (target && target->Resource) {
				D3D12_RESOURCE_DESC desc = target->Resource->GetDesc();
				width = static_cast<unsigned int>(desc.Width);
				height = static_cast<unsigned int>(desc.Height);
			}
			else {
				width = backBufferWidth;
				height = backBufferHeight;
			}
		}
	}

	void RenderTargetManager::Resize(unsigned int width, unsigned int height)
	{
		if (width == 0 || height == 0) {
			return;
		}
		m_SwapChainResizePending = true;
		m_SwapChainPendingWidth = width;
		m_SwapChainPendingHeight = height;
	}

	void RenderTargetManager::ResizeTarget(RenderTargetType type, unsigned int width, unsigned int height)
	{
		if (width == 0 || height == 0) return;

		if (type == RenderTargetType::GAME_VIEW) {
			m_GameViewResizePending = true;
			m_GameViewPendingWidth = width;
			m_GameViewPendingHeight = height;
		}
		else if (type == RenderTargetType::SCENE_VIEW) {
			m_SceneViewResizePending = true;
			m_SceneViewPendingWidth = width;
			m_SceneViewPendingHeight = height;
		}
	}

	void RenderTargetManager::ApplyPendingResizes(ID3D12Device* device, IDXGISwapChain3* swapChain, DescriptorAllocator& srvAllocator, DescriptorAllocator& rtvAllocator, int& backBufferWidth, int& backBufferHeight)
	{
		if (m_SwapChainResizePending) {
			m_BackBuffer[0].Reset();
			m_BackBuffer[1].Reset();
			m_DepthBuffer.Reset();

			DXGI_SWAP_CHAIN_DESC1 desc = {};
			HRESULT hr = swapChain->GetDesc1(&desc);
			assert(SUCCEEDED(hr));

			hr = swapChain->ResizeBuffers(0, m_SwapChainPendingWidth, m_SwapChainPendingHeight, desc.Format, desc.Flags);
			assert(SUCCEEDED(hr));

			backBufferWidth = static_cast<int>(m_SwapChainPendingWidth);
			backBufferHeight = static_cast<int>(m_SwapChainPendingHeight);

			m_Viewport.TopLeftX = 0.0f;
			m_Viewport.TopLeftY = 0.0f;
			m_Viewport.Width = static_cast<FLOAT>(m_SwapChainPendingWidth);
			m_Viewport.Height = static_cast<FLOAT>(m_SwapChainPendingHeight);
			m_Viewport.MinDepth = 0.0f;
			m_Viewport.MaxDepth = 1.0f;

			m_ScissorRect.left = 0;
			m_ScissorRect.top = 0;
			m_ScissorRect.right = static_cast<LONG>(m_SwapChainPendingWidth);
			m_ScissorRect.bottom = static_cast<LONG>(m_SwapChainPendingHeight);

			CreateBackBufferTargets(device, swapChain);

			CreateDepthBuffer(device, 1920, 1080);

			FLOAT colorClear[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
			FLOAT zeroClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			m_ColorBuffer.reset();
			m_ColorBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, colorClear);
			m_ColorBuffer->Resource->SetName(L"ColorBuffer");

			m_NormalBuffer.reset();
			m_NormalBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
			m_NormalBuffer->Resource->SetName(L"NormalBuffer");

			m_PositionBuffer.reset();
			m_PositionBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
			m_PositionBuffer->Resource->SetName(L"PositionBuffer");

			m_MaterialBuffer.reset();
			m_MaterialBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
			m_MaterialBuffer->Resource->SetName(L"MaterialBuffer");

			m_EmissionBuffer.reset();
			m_EmissionBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
			m_EmissionBuffer->Resource->SetName(L"EmissionBuffer");

			m_LightedColorBuffer.reset();
			m_LightedColorBuffer = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_LightedColorBuffer->Resource->SetName(L"LightedColorBuffer");

			m_GameViewTarget.reset();
			m_GameViewTarget = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, m_SwapChainPendingWidth, m_SwapChainPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_GameViewTarget->Resource->SetName(L"GameViewTarget");

			m_SceneViewTarget.reset();
			m_SceneViewTarget = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, m_SwapChainPendingWidth, m_SwapChainPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_SceneViewTarget->Resource->SetName(L"SceneViewTarget");

			m_SwapChainResizePending = false;
			m_GameViewResizePending = false;
			m_SceneViewResizePending = false;
		}
		else {
			if (m_GameViewResizePending) {
				m_GameViewTarget.reset();
				m_GameViewTarget = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, m_GameViewPendingWidth, m_GameViewPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
				m_GameViewTarget->Resource->SetName(L"GameViewTarget");
				m_GameViewResizePending = false;
			}

			if (m_SceneViewResizePending) {
				m_SceneViewTarget.reset();
				m_SceneViewTarget = RenderTargetFactory::Create(device, srvAllocator, rtvAllocator, m_SceneViewPendingWidth, m_SceneViewPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
				m_SceneViewTarget->Resource->SetName(L"SceneViewTarget");
				m_SceneViewResizePending = false;
			}
		}
	}

}
