#include "../Manager/Main.h"
#include "GraphicsDevice.h"
#include <cassert>

namespace EngineCore::Render {

	void GraphicsDevice::Init(HWND windowHandle, bool windowMode, unsigned int width, unsigned int height)
	{
		HRESULT hr;

#if defined(_DEBUG)
		// DebugLayer
		{
			ComPtr<ID3D12Debug1> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
#endif

		// DXGI Factory
		{
			UINT flag{};
			hr = CreateDXGIFactory2(flag, IID_PPV_ARGS(&m_Factory));
			assert(SUCCEEDED(hr));

			hr = m_Factory->EnumAdapters(0, (IDXGIAdapter**)m_Adapter.GetAddressOf());
			assert(SUCCEEDED(hr));

			hr = D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_Device));
			assert(SUCCEEDED(hr));
		}

		// CommandQueue
		{
			D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

			commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			commandQueueDesc.Priority = 0;
			commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			commandQueueDesc.NodeMask = 0;

			hr = m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue));
			assert(SUCCEEDED(hr));

			m_FenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
			assert(m_FenceEvent);

			hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
			assert(SUCCEEDED(hr));
		}

		// CommandAllocator
		{
			hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_GraphicsCommandAllocator[0]));
			assert(SUCCEEDED(hr));
			m_GraphicsCommandAllocator[0]->SetName(L"GraphicsCommandAllocator[0]");

			hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_GraphicsCommandAllocator[1]));
			assert(SUCCEEDED(hr));
			m_GraphicsCommandAllocator[1]->SetName(L"GraphicsCommandAllocator[1]");

			hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_GraphicsCommandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&m_GraphicsCommandList));
			assert(SUCCEEDED(hr));
			m_GraphicsCommandList->SetName(L"GraphicsCommandList");
		}

		// SwapChain
		{
			DXGI_SWAP_CHAIN_DESC swapChainDesc{};
			ComPtr<IDXGISwapChain> swapChain{};

			swapChainDesc.BufferDesc.Width = width;
			swapChainDesc.BufferDesc.Height = height;
			swapChainDesc.OutputWindow = windowHandle;
			swapChainDesc.Windowed = windowMode;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = 2;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.Flags = 0;
			swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
			swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;

			hr = m_Factory->CreateSwapChain(m_CommandQueue.Get(), &swapChainDesc, &swapChain);
			assert(SUCCEEDED(hr));

			hr = swapChain.As(&m_SwapChain);
			assert(SUCCEEDED(hr));
		}
	}

}
