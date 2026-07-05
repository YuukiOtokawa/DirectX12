#pragma once

#include "Main.h"

namespace EngineCore::Render {

	// D3D12デバイス・コマンドキュー・フェンス・スワップチェーン・コマンドリストの初期化をまとめる
	class GraphicsDevice
	{
	public:
		void Init(HWND windowHandle, bool windowMode, unsigned int width, unsigned int height);

		IDXGIFactory4* GetFactory() const { return m_Factory.Get(); }
		IDXGIAdapter3* GetAdapter() const { return m_Adapter.Get(); }
		ID3D12Device* GetDevice() const { return m_Device.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }
		ID3D12Fence* GetFence() const { return m_Fence.Get(); }
		HANDLE GetFenceEvent() const { return m_FenceEvent; }
		IDXGISwapChain3* GetSwapChain() const { return m_SwapChain.Get(); }
		ID3D12GraphicsCommandList* GetGraphicsCommandList() const { return m_GraphicsCommandList.Get(); }
		ID3D12CommandAllocator* GetCommandAllocator(unsigned int index) const { return m_GraphicsCommandAllocator[index].Get(); }

	private:
		ComPtr<IDXGIFactory4>				m_Factory;
		ComPtr<IDXGIAdapter3>				m_Adapter;
		ComPtr<ID3D12Device>				m_Device;
		ComPtr<ID3D12CommandQueue>			m_CommandQueue;
		ComPtr<ID3D12Fence>					m_Fence;
		HANDLE								m_FenceEvent = nullptr;
		ComPtr<IDXGISwapChain3>				m_SwapChain;
		ComPtr<ID3D12GraphicsCommandList>	m_GraphicsCommandList;
		ComPtr<ID3D12CommandAllocator>		m_GraphicsCommandAllocator[2];
	};

}
