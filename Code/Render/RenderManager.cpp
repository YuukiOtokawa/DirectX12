#include "Main.h"
#include "RenderManager.h"

#include "D3DX12.h"
#include "DDSTextureLoader12.h"
#include "dxgiformat.h"
#include <d3dcompiler.h>
#include <algorithm>
#pragma comment(lib, "d3dcompiler.lib")
using namespace DirectX;

using namespace EngineCore;
using namespace Render;


RenderManager* RenderManager::m_Instance = nullptr;




RenderManager::RenderManager()
{
	m_Instance = this;

	Init();
}




RenderManager::~RenderManager()
{
	WaitGPU();
/*
#if defined(_DEBUG)
	//ReportLiveDeviceObjects
	{
		ComPtr<ID3D12DebugDevice> debugInterface;

		if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&debugInterface))))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
#endif
*/
}



//==================================================
// DirectX 12 Initialization
//==================================================

void RenderManager::Init()
{
    // Window and BackBuffer settings

	m_WindowMode = true;

	m_BackBufferWidth = 1280;
	m_BackBufferHeight = 720;



	m_WindowHandle = GetWindow();
	m_Frame[0] = 0;
	m_Frame[1] = 0;
	m_FenceValue = 0;
	m_RTIndex = 0;


	m_Viewport.TopLeftX = 0.f;
	m_Viewport.TopLeftY = 0.f;
	m_Viewport.Width = (FLOAT)m_BackBufferWidth;
	m_Viewport.Height = (FLOAT)m_BackBufferHeight;
	m_Viewport.MinDepth = 0.f;
	m_Viewport.MaxDepth = 1.f;


	// ScissorRect
	m_ScissorRect.top = 0;
	m_ScissorRect.left = 0;
	m_ScissorRect.right = m_BackBufferWidth;
	m_ScissorRect.bottom = m_BackBufferHeight;






	HRESULT hr;



#if defined(_DEBUG)

	// DebugLayer
	{
		ComPtr<ID3D12Debug1>	debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			//debugController->SetEnableGPUBasedValidation(true);
		}
	}

	
/*	
	//DRED
	{
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> d3dDredSettings1;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDredSettings1))))
		{
			d3dDredSettings1->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			d3dDredSettings1->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			d3dDredSettings1->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		}
	}
*/
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

		swapChainDesc.BufferDesc.Width = m_BackBufferWidth;
		swapChainDesc.BufferDesc.Height = m_BackBufferHeight;
		swapChainDesc.OutputWindow = m_WindowHandle;
		swapChainDesc.Windowed = m_WindowMode;
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

		m_RTIndex = m_SwapChain->GetCurrentBackBufferIndex();
	}





	// RenderTargetDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.NumDescriptors = 2;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		hr = m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_RenderTargetDescriptorHeap));
		assert(SUCCEEDED(hr));

		UINT size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (UINT i = 0; i < 2; ++i)
		{
			hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTarget[i]));
			assert(SUCCEEDED(hr));

			m_RenderTarget[i]->SetName(L"RenderTarget");

			m_RenderTargetHandle[i] = m_RenderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_RenderTargetHandle[i].ptr += size * i;
			m_Device->CreateRenderTargetView(m_RenderTarget[i].Get(), nullptr, m_RenderTargetHandle[i]);
		}
	}







	// DepthBufferDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.NumDescriptors = 1;///////////////////////////////////
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapDesc.NodeMask = 0;

		hr = m_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DepthBufferDescriptorHeap));
		assert(SUCCEEDED(hr));
	}




	// DepthBuffer
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = m_BackBufferWidth;
		resourceDesc.Height = m_BackBufferHeight;
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

		hr = m_Device->CreateCommittedResource(&prop,
												D3D12_HEAP_FLAG_NONE,
												&resourceDesc,
												D3D12_RESOURCE_STATE_DEPTH_WRITE,
												&clearValue,
												IID_PPV_ARGS(&m_DepthBuffer));
		assert(SUCCEEDED(hr));

		m_DepthBuffer->SetName(L"DepthBuffer");



		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_DepthBufferHandle = m_DepthBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		m_Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DepthBufferHandle);

	}






	// ShaderVisibleDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NumDescriptors = SRV_DESCRIPTOR_MAX;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NodeMask = 0;

		m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SRVDescriptorHeap));

		// 汎用プールは予約領域(MATERIAL_BLOCK_REGION_BASE)の手前まで
		for (unsigned int i = 0; i < MATERIAL_BLOCK_REGION_BASE; i++)
			m_SRVDescriptorPool.push_back(i);

		// マテリアルブロック専用領域を8枠刻みでブロックプールへ
		for (unsigned int b = 0; b < MATERIAL_BLOCK_MAX; b++)
			m_MaterialBlockPool.push_back(MATERIAL_BLOCK_REGION_BASE + b * MATERIAL_TEX_SLOTS);
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = RTV_DESCRIPTOR_MAX;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NodeMask = 0;

		m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_RTVDescriptorHeap));

		for (int i = 0; i < RTV_DESCRIPTOR_MAX; i++)
			m_RTVDescriptorPool.push_back(i);
	}




	// ConstantBuffer
	for (int i = 0; i < 2; i++)
	{
		{
			D3D12_HEAP_PROPERTIES properties{};
			properties.Type = D3D12_HEAP_TYPE_UPLOAD;
			properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			properties.CreationNodeMask = 0;
			properties.VisibleNodeMask = 0;

			D3D12_RESOURCE_DESC desc{};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Width = CONSTANT_BUFFER_SIZE * CONSTANT_BUFFER_MAX;


			HRESULT hr = m_Device->CreateCommittedResource(&properties,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_ConstantBuffer[i]));
			assert(SUCCEEDED(hr));
		}


		hr = m_ConstantBuffer[i]->Map(0, nullptr, (void**)&m_ConstantBufferPointer[i]);
		assert(SUCCEEDED(hr));


		for (int j = 0; j < CONSTANT_BUFFER_MAX; j++)
		{
			unsigned int index = m_SRVDescriptorPool.front();
			m_SRVDescriptorPool.pop_front();




			D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
			desc.BufferLocation = m_ConstantBuffer[i]->GetGPUVirtualAddress() + j * CONSTANT_BUFFER_SIZE;
			desc.SizeInBytes = CONSTANT_BUFFER_SIZE;



			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			handle.ptr += size * index;

			m_Device->CreateConstantBufferView(&desc, handle);

			m_ConstantBufferView[i][j] = index;
		}

		m_ConstantBufferIndex[i] = 0;
	}





	// RootSignature
	{

		D3D12_ROOT_PARAMETER		rootParameters[13]{};
		D3D12_DESCRIPTOR_RANGE		range[13]{};


		// ConstantBuffer
		for (unsigned int i = 0; i < 4; i++)
		{
			range[i].NumDescriptors = 1;
			range[i].BaseShaderRegister = i;
			range[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[i].DescriptorTable.pDescriptorRanges = &range[i];
		}


		// SRV
		for (unsigned int i = 4; i < 12; i++)
		{
			range[i].NumDescriptors = 1;
			range[i].BaseShaderRegister = i - 4;
			range[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[i].DescriptorTable.pDescriptorRanges = &range[i];
		}


		// MaterialTexture (register space1, t0..t7 を1つの連続テーブルとして)
		{
			unsigned int i = MATERIAL_TEX_ROOT_PARAM; // = 12
			range[i].NumDescriptors = MATERIAL_TEX_SLOTS; // 8
			range[i].BaseShaderRegister = 0;
			range[i].RegisterSpace = 1;                   // ← space1
			range[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[i].DescriptorTable.pDescriptorRanges = &range[i];
		}


		// StaticSampler
		D3D12_STATIC_SAMPLER_DESC	samplerDesc[2]{};
		//samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc[0].Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc[0].MipLODBias = 0.0f;
		samplerDesc[0].MaxAnisotropy = 4;
		samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplerDesc[0].MinLOD = 0.0f;
		samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc[0].ShaderRegister = 0;
		samplerDesc[0].RegisterSpace = 0;
		samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		samplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc[1].MipLODBias = 0.0f;
		samplerDesc[1].MaxAnisotropy = 16;
		samplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplerDesc[1].MinLOD = 0.0f;
		samplerDesc[1].MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc[1].ShaderRegister = 1;
		samplerDesc[1].RegisterSpace = 0;
		samplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;



		D3D12_ROOT_SIGNATURE_DESC	rootSignatureDesc{};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = 2;
		rootSignatureDesc.pStaticSamplers = samplerDesc;


		HRESULT hr;
		ComPtr<ID3DBlob> blob{};
		hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
		assert(SUCCEEDED(hr));

		hr = m_Device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
		assert(SUCCEEDED(hr));
	}





	// VertexBuffer
	{
		m_VertexBuffer = CreateVertexBuffer(sizeof(VERTEX), 4);

		// Map
		VERTEX* buffer{};
		hr = m_VertexBuffer->Resource->Map(0, nullptr, (void**)&buffer);
		assert(SUCCEEDED(hr));

		buffer[0].Position = Vector3(-1.0f,  1.0f,  0.0f);
		buffer[1].Position = Vector3(1.0f,  1.0f, 0.0f);
		buffer[2].Position = Vector3(-1.0f,  -1.0f, 0.0f);
		buffer[3].Position = Vector3(1.0f,  -1.0f, 0.0f);

		buffer[0].Color = Vector4(1.0f,  1.0f,  1.0f, 1.0f);
		buffer[1].Color = Vector4(1.0f,  1.0f,  1.0f, 1.0f);
		buffer[2].Color = Vector4(1.0f,  1.0f,  1.0f, 1.0f);
		buffer[3].Color = Vector4(1.0f,  1.0f,  1.0f, 1.0f);

		buffer[0].Normal = Vector3(0.0f, 1.0f, 0.0f);
		buffer[1].Normal = Vector3(0.0f, 1.0f, 0.0f);
		buffer[2].Normal = Vector3(0.0f, 1.0f, 0.0f);
		buffer[3].Normal = Vector3(0.0f, 1.0f, 0.0f);

		buffer[0].TexCoord = Vector2(0.0f, 0.0f);
		buffer[1].TexCoord = Vector2(1.0f, 0.0f);
		buffer[2].TexCoord = Vector2(0.0f, 1.0f);
		buffer[3].TexCoord = Vector2(1.0f, 1.0f);

		m_VertexBuffer->Resource->Unmap(0, nullptr);
	}

	// PipelineState
	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };

		m_PipelineState["Unlit"] = CreatePipeline("Code/Shader/Unlit.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::ForwardOpaque);

	}

	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };

		m_PipelineState["Screen"] = CreatePipeline("Code/Shader/Screen.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);

	}


	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		m_PipelineState["Deferred"] = CreatePipeline("Code/Shader/Deferred.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	}

	{
		DXGI_FORMAT RTVFormats[] = {
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		};
		m_PipelineState["Geometry"] = CreatePipeline("Code/Shader/Geometry.hlsl", RTVFormats, _countof(RTVFormats));
	}

	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		m_PipelineState["PostProcess"] = CreatePipeline("Code/Shader/PostProcess.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	}

	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		m_PipelineState["InvertColor"] = CreatePipeline("Code/Shader/InvertColor.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	}

	{
		FLOAT colorClear[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		FLOAT zeroClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_ColorBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, colorClear);
		m_ColorBuffer->Resource->SetName(L"ColorBuffer");

		m_NormalBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_NormalBuffer->Resource->SetName(L"NormalBuffer");

		m_PositionBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_PositionBuffer->Resource->SetName(L"PositionBuffer");

		m_MaterialBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_MaterialBuffer->Resource->SetName(L"MaterialBuffer");

		m_EmissionBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_EmissionBuffer->Resource->SetName(L"EmissionBuffer");

		m_LightedColorBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_LightedColorBuffer->Resource->SetName(L"LightedColorBuffer");

		m_PostProcessBuffer1 = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_PostProcessBuffer1->Resource->SetName(L"PostProcessBuffer1");
	}

	m_EnvTexture = LoadTexture("Assets\\charolettenbrunn_park_2k.dds");
}






//==================================================
// Wait for GPU completion
//==================================================

void RenderManager::WaitGPU()
{
	m_FenceValue++;

	m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue);

	m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
}









//==================================================
// Begin drawing
//==================================================

void RenderManager::DrawBegin()
{
	// Descriptor heaps
	ID3D12DescriptorHeap* dh[] = { m_SRVDescriptorHeap.Get() };
	m_GraphicsCommandList->SetDescriptorHeaps(_countof(dh), dh);

	// Root signature
	m_GraphicsCommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// マテリアルテクスチャ（space1）の初期化を保証（初回のみ実行）
	EnsureMaterialTextureSetup();

	// Constant buffer index reset
	m_ConstantBufferIndex[m_RTIndex] = 0;

	if (m_CurrentTargetType == RENDER_TARGET_TYPE::BACK_BUFFER) {
		m_GraphicsCommandList->RSSetViewports(1, &m_Viewport);
		m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

		// Transition back buffer: PRESENT -> RENDER_TARGET
		{
			auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
				m_RenderTarget[m_RTIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(1, &trans);
		}

		m_GraphicsCommandList->OMSetRenderTargets(1, &m_RenderTargetHandle[m_RTIndex], TRUE, &m_DepthBufferHandle);

		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(m_RenderTargetHandle[m_RTIndex], clearColor, 0, nullptr);
		m_GraphicsCommandList->ClearDepthStencilView(m_DepthBufferHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
	else {
		// Set G-Buffer pass viewports and scissors to fixed 1920x1080 size
		D3D12_VIEWPORT gbufferViewport{};
		gbufferViewport.TopLeftX = 0.0f;
		gbufferViewport.TopLeftY = 0.0f;
		gbufferViewport.Width = 1920.0f;
		gbufferViewport.Height = 1080.0f;
		gbufferViewport.MinDepth = 0.0f;
		gbufferViewport.MaxDepth = 1.0f;

		D3D12_RECT gbufferScissor{};
		gbufferScissor.left = 0;
		gbufferScissor.top = 0;
		gbufferScissor.right = 1920;
		gbufferScissor.bottom = 1080;

		m_GraphicsCommandList->RSSetViewports(1, &gbufferViewport);
		m_GraphicsCommandList->RSSetScissorRects(1, &gbufferScissor);

		// Transition G-Buffer: PIXEL_SHADER_RESOURCE -> RENDER_TARGET
		{
			D3D12_RESOURCE_BARRIER barriers[5];
			barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
				m_ColorBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
				m_NormalBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(
				m_PositionBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(
				m_MaterialBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(
				m_EmissionBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(5, barriers);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[] =
		{
			m_ColorBuffer->RTVHandle,
			m_NormalBuffer->RTVHandle,
			m_PositionBuffer->RTVHandle,
			m_MaterialBuffer->RTVHandle,
			m_EmissionBuffer->RTVHandle
		};
		m_GraphicsCommandList->OMSetRenderTargets(_countof(renderTargets), renderTargets, false, &m_DepthBufferHandle);

		FLOAT clearColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		FLOAT clearNormal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_GraphicsCommandList->ClearRenderTargetView(m_ColorBuffer->RTVHandle, clearColor, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(m_NormalBuffer->RTVHandle, clearNormal, 0, nullptr);
        m_GraphicsCommandList->ClearRenderTargetView(
            m_PositionBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(m_MaterialBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(m_EmissionBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearDepthStencilView(m_DepthBufferHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

//==================================================
// End drawing
//==================================================

void RenderManager::DrawEnd()
{
	if (m_CurrentTargetType == RENDER_TARGET_TYPE::BACK_BUFFER) {
		// BackBuffer path: ImGui will render onto it, so we do nothing here.
	}
	else {
		RENDER_TARGET* target = (m_CurrentTargetType == RENDER_TARGET_TYPE::GAME_VIEW) ? m_GameViewTarget.get() : m_SceneViewTarget.get();
		if (target) {
			// 1) Transition m_LightedColorBuffer: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
			{
				auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
					m_LightedColorBuffer->Resource.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				m_GraphicsCommandList->ResourceBarrier(1, &trans);
			}

			// 2) Transition target: PIXEL_SHADER_RESOURCE -> RENDER_TARGET
			{
				auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
					target->Resource.Get(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET);
				m_GraphicsCommandList->ResourceBarrier(1, &trans);
			}

			// 3) Set target RTV and clear it
			m_GraphicsCommandList->OMSetRenderTargets(1, &target->RTVHandle, TRUE, nullptr);

			FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			m_GraphicsCommandList->ClearRenderTargetView(target->RTVHandle, clearColor, 0, nullptr);

			// 4) Set dynamic viewport for aspect ratio / scale copy
			D3D12_VIEWPORT targetViewport = m_Viewport;
			D3D12_RECT targetScissorRect = m_ScissorRect;
			if (target->Resource) {
				D3D12_RESOURCE_DESC desc = target->Resource->GetDesc();
				targetViewport.Width = static_cast<FLOAT>(desc.Width);
				targetViewport.Height = static_cast<FLOAT>(desc.Height);
				targetScissorRect.right = static_cast<LONG>(desc.Width);
				targetScissorRect.bottom = static_cast<LONG>(desc.Height);
			}
			m_GraphicsCommandList->RSSetViewports(1, &targetViewport);
			m_GraphicsCommandList->RSSetScissorRects(1, &targetScissorRect);

			// 5) Render screen copy with scaling (1920x1080 lighted buffer -> target resolution)
			SetPipelineState("Screen");
			SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, m_LightedColorBuffer.get());
			DrawScreen();

			// 6) Transition target: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
			{
				auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
					target->Resource.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				m_GraphicsCommandList->ResourceBarrier(1, &trans);
			}

			// 7) Transition m_LightedColorBuffer: PIXEL_SHADER_RESOURCE -> RENDER_TARGET
			{
				auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
					m_LightedColorBuffer->Resource.Get(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET);
				m_GraphicsCommandList->ResourceBarrier(1, &trans);
			}
		}
	}
}

//==================================================
// Deferred Lighting Pass
//==================================================

void RenderManager::ResolveDeferredLighting()
{
	if (m_CurrentTargetType == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}

	// 1) Transit G-Buffer: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
	{
		D3D12_RESOURCE_BARRIER barriers[5];
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_ColorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_NormalBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_PositionBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_MaterialBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_EmissionBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(5, barriers);
	}

	// 2) Render deferred lighting directly into m_LightedColorBuffer (1920x1080).
	//    Post-process is applied later in ApplyPostProcess(), after the forward pass,
	//    so that both deferred and forward geometry receive the post effects.
	//    m_LightedColorBuffer is already in RENDER_TARGET state (steady-state contract).
	{
		m_GraphicsCommandList->OMSetRenderTargets(1, &m_LightedColorBuffer->RTVHandle, TRUE, nullptr);

		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(m_LightedColorBuffer->RTVHandle, clearColor, 0, nullptr);

		// Set G-Buffer pass viewports and scissors to fixed 1920x1080 size
		D3D12_VIEWPORT gbufferViewport{};
		gbufferViewport.TopLeftX = 0.0f;
		gbufferViewport.TopLeftY = 0.0f;
		gbufferViewport.Width = 1920.0f;
		gbufferViewport.Height = 1080.0f;
		gbufferViewport.MinDepth = 0.0f;
		gbufferViewport.MaxDepth = 1.0f;

		D3D12_RECT gbufferScissor{};
		gbufferScissor.left = 0;
		gbufferScissor.top = 0;
		gbufferScissor.right = 1920;
		gbufferScissor.bottom = 1080;

		m_GraphicsCommandList->RSSetViewports(1, &gbufferViewport);
		m_GraphicsCommandList->RSSetScissorRects(1, &gbufferScissor);

		// Deferred Shading / Lighting
		SetPipelineState("Deferred");
		SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, m_ColorBuffer.get());
		SetTexture(RenderManager::TEXTURE_TYPE::NORMAL, m_NormalBuffer.get());
		SetTexture(RenderManager::TEXTURE_TYPE::POSITION, m_PositionBuffer.get());
		SetTexture(RenderManager::TEXTURE_TYPE::MATERIAL, m_MaterialBuffer.get());
		SetTexture(RenderManager::TEXTURE_TYPE::EMISSION, m_EmissionBuffer.get());
		SetTexture(RenderManager::TEXTURE_TYPE::ENVIRONMENT, m_EnvTexture.get());
		DrawScreen();
	}
}

//==================================================
// Apply Post-Process Passes
//==================================================

void RenderManager::ApplyPostProcess()
{
	if (m_CurrentTargetType == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}

	// At this point m_LightedColorBuffer holds the fully composited scene
	// (deferred lighting + forward geometry) and is in RENDER_TARGET state.
	// With no passes registered there is nothing to do: DrawEnd() will copy it as-is.
	if (m_ActivePostProcessPasses.empty()) {
		return;
	}

	// Set 1920x1080 viewport / scissor for the full-screen passes.
	D3D12_VIEWPORT ppViewport{};
	ppViewport.TopLeftX = 0.0f;
	ppViewport.TopLeftY = 0.0f;
	ppViewport.Width = 1920.0f;
	ppViewport.Height = 1080.0f;
	ppViewport.MinDepth = 0.0f;
	ppViewport.MaxDepth = 1.0f;

	D3D12_RECT ppScissor{};
	ppScissor.left = 0;
	ppScissor.top = 0;
	ppScissor.right = 1920;
	ppScissor.bottom = 1080;

	m_GraphicsCommandList->RSSetViewports(1, &ppViewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &ppScissor);

	// Ping-pong between m_LightedColorBuffer (source/result) and m_PostProcessBuffer1 (scratch).
	RENDER_TARGET* currentInput = m_LightedColorBuffer.get();
	RENDER_TARGET* currentOutput = m_PostProcessBuffer1.get();

	// Transition the composited image (input): RENDER_TARGET -> PIXEL_SHADER_RESOURCE
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			currentInput->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(1, &barrier);
	}

	for (size_t i = 0; i < m_ActivePostProcessPasses.size(); ++i) {
		const auto& pass = m_ActivePostProcessPasses[i];

		// Output must be in RENDER_TARGET state (both buffers rest in PIXEL_SHADER_RESOURCE).
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				currentOutput->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(1, &barrier);
		}

		m_GraphicsCommandList->OMSetRenderTargets(1, &currentOutput->RTVHandle, TRUE, nullptr);
		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(currentOutput->RTVHandle, clearColor, 0, nullptr);

		SetPipelineState(pass.name.c_str());
		SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, currentInput);
		// パス独自のマテリアルプロパティ(b3)があればバインド
		if (!pass.propertyBuffer.empty()) {
			SetConstant(RenderManager::CONSTANT_TYPE::SUBSET,
				pass.propertyBuffer.data(),
				static_cast<unsigned int>(pass.propertyBuffer.size()));
		}
		DrawScreen();

		// This pass's output becomes the next pass's input.
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				currentOutput->Resource.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_GraphicsCommandList->ResourceBarrier(1, &barrier);
		}

		std::swap(currentInput, currentOutput);
	}

	// After the loop the final result is in currentInput (PIXEL_SHADER_RESOURCE state).
	// DrawEnd() expects the result in m_LightedColorBuffer in RENDER_TARGET state.
	if (currentInput == m_LightedColorBuffer.get()) {
		// Even number of passes: result already lives in m_LightedColorBuffer.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_LightedColorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_GraphicsCommandList->ResourceBarrier(1, &barrier);
	}
	else {
		// Odd number of passes: result lives in m_PostProcessBuffer1; copy it back.
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_LightedColorBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(1, &barrier);
		}

		m_GraphicsCommandList->OMSetRenderTargets(1, &m_LightedColorBuffer->RTVHandle, TRUE, nullptr);
		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(m_LightedColorBuffer->RTVHandle, clearColor, 0, nullptr);

		SetPipelineState("PostProcess");
		SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, currentInput);
		DrawScreen();
		// m_PostProcessBuffer1 remains in PIXEL_SHADER_RESOURCE (its resting state).
	}
}

//==================================================
// Begin Forward Pass
//==================================================

void RenderManager::BeginForwardPass()
{
	if (m_CurrentTargetType == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}

	// Set m_LightedColorBuffer RTV and preserve existing 1920x1080 depth buffer (DSV)
	m_GraphicsCommandList->OMSetRenderTargets(1, &m_LightedColorBuffer->RTVHandle, TRUE, &m_DepthBufferHandle);

	// Set G-Buffer pass viewports and scissors to fixed 1920x1080 size
	D3D12_VIEWPORT gbufferViewport{};
	gbufferViewport.TopLeftX = 0.0f;
	gbufferViewport.TopLeftY = 0.0f;
	gbufferViewport.Width = 1920.0f;
	gbufferViewport.Height = 1080.0f;
	gbufferViewport.MinDepth = 0.0f;
	gbufferViewport.MaxDepth = 1.0f;

	D3D12_RECT gbufferScissor{};
	gbufferScissor.left = 0;
	gbufferScissor.top = 0;
	gbufferScissor.right = 1920;
	gbufferScissor.bottom = 1080;

	m_GraphicsCommandList->RSSetViewports(1, &gbufferViewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &gbufferScissor);
}

void RenderManager::FrameEnd() {
	// Transition back buffer: RENDER_TARGET -> PRESENT
	{
		auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
			m_RenderTarget[m_RTIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		m_GraphicsCommandList->ResourceBarrier(1, &trans);
	}

	// Execute command lists
	{
		HRESULT hr = m_GraphicsCommandList->Close();
		assert(SUCCEEDED(hr));

		ID3D12CommandList* const command_lists[1] = { m_GraphicsCommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(1, command_lists);

		// Increment global fence value and signal it for the current back buffer
		m_FenceValue++;
		m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue);
		m_Frame[m_RTIndex] = m_FenceValue;
	}

	// Swap buffers and wait for the target back buffer's GPU completion
	{
		HRESULT hr = m_SwapChain->Present(1, 0);
		assert(SUCCEEDED(hr));

		m_RTIndex = m_SwapChain->GetCurrentBackBufferIndex();


		if (m_Fence->GetCompletedValue() < m_Frame[m_RTIndex]) {
			m_Fence->SetEventOnCompletion(m_Frame[m_RTIndex], m_FenceEvent);
			WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
		}

		// GPU側で実行が完了したPSOを解放する
		UINT64 completedValue = m_Fence->GetCompletedValue();
		m_PendingReleasePSOs.erase(
			std::remove_if(m_PendingReleasePSOs.begin(), m_PendingReleasePSOs.end(),
				[completedValue](const PendingReleasePSO& pending) {
					return completedValue >= pending.fenceValue;
				}),
			m_PendingReleasePSOs.end()
		);

		// GPU側で実行が完了したテクスチャを解放する
		m_PendingReleaseTextures.erase(
			std::remove_if(m_PendingReleaseTextures.begin(), m_PendingReleaseTextures.end(),
				[completedValue](const PendingReleaseTexture& pending) {
					return completedValue >= pending.fenceValue;
				}),
			m_PendingReleaseTextures.end()
		);
	}




	HRESULT hr = m_GraphicsCommandAllocator[m_RTIndex]->Reset();
	assert(SUCCEEDED(hr));

	hr = m_GraphicsCommandList->Reset(m_GraphicsCommandAllocator[m_RTIndex].Get(), m_PipelineState["Deferred"].Get());
	assert(SUCCEEDED(hr));

}

void RenderManager::DrawScreen()
{



	//頂点バッファ設定
	SetVertexBuffer(m_VertexBuffer.get());


	//トポロジー設定
	m_GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	//描画
	m_GraphicsCommandList->DrawInstanced(4, 1, 0, 0);


}





std::unique_ptr<TEXTURE> RenderManager::LoadTexture(const char* FileName)
{
	std::unique_ptr<TEXTURE> texture = std::make_unique<TEXTURE>();


	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresouceData;

	wchar_t wFileName[MAX_PATH];
	size_t size;
	mbstowcs_s(&size, wFileName, FileName, MAX_PATH);

	HRESULT hr = LoadDDSTextureFromFile(m_Device.Get(), wFileName, &texture->Resource, ddsData, subresouceData);
	assert(SUCCEEDED(hr));

	texture->Resource->SetName(wFileName);



	D3D12_RESOURCE_DESC desc = texture->Resource->GetDesc();

	unsigned int bpp, block;

	if (desc.Format == DXGI_FORMAT_BC1_UNORM)
	{
		bpp = 4;
		block = 4;
	}
	else if (desc.Format == DXGI_FORMAT_BC6H_UF16)
	{
		bpp = 8;
		block = 4;
	}
	else
	{
		bpp = 32;
		block = 1;
	}
	




	for (unsigned int a = 0; a < desc.DepthOrArraySize; a++)
	{
		for (unsigned int m = 0; m < desc.MipLevels; m++)
		{
			unsigned int s = a * desc.MipLevels + m;

			unsigned int width = (unsigned int)subresouceData[s].RowPitch * 8 / bpp / block;
			unsigned int height = (unsigned int)subresouceData[s].SlicePitch / (unsigned int)subresouceData[s].RowPitch * block;

			D3D12_BOX box = { 0, 0, 0, width, height, 1 };

			hr = texture->Resource->WriteToSubresource(s, &box, subresouceData[s].pData, (UINT)subresouceData[s].RowPitch, (UINT)subresouceData[s].SlicePitch);
			assert(SUCCEEDED(hr));
		}
	}

	auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
		texture->Resource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_GraphicsCommandList->ResourceBarrier(1, &trans);





	texture->SRVIndex = CreateShaderResourceView(texture->Resource.Get());



	return std::move(texture);
}






ComPtr<ID3D12PipelineState> RenderManager::CreatePipeline(const char* ShaderFile, const DXGI_FORMAT* RTVFormats, unsigned int NumRenderTargets, RenderPassType passType)
{

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};

	auto compileShader = [](const char* filename, const char* entrypoint, const char* target, ID3DBlob** blob) -> bool {
		wchar_t wFileName[MAX_PATH];
		size_t size;
		mbstowcs_s(&size, wFileName, filename, MAX_PATH);

		UINT compileFlags = 0;
#if defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3DCompileFromFile(
			wFileName,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entrypoint,
			target,
			compileFlags,
			0,
			blob,
			&errorBlob
		);

		if (FAILED(hr)) {
			if (errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "Shader Compilation Error", MB_OK | MB_ICONERROR);
			}
			return false;
		}
		return true;
	};

	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	// register space を使うため Shader Model 5.1 でコンパイル（5.0の上位互換）
	bool vsSuccess = compileShader(ShaderFile, "vtx", "vs_5_1", &vsBlob);
	if (!vsSuccess) return nullptr;
	bool psSuccess = compileShader(ShaderFile, "pix", "ps_5_1", &psBlob);
	if (!psSuccess) return nullptr;

	pipelineStateDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineStateDesc.VS.BytecodeLength = vsBlob->GetBufferSize();

	pipelineStateDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineStateDesc.PS.BytecodeLength = psBlob->GetBufferSize();


	//インプットレイアウト
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipelineStateDesc.InputLayout.pInputElementDescs = InputElementDesc;
	pipelineStateDesc.InputLayout.NumElements = _countof(InputElementDesc);



	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleDesc.Quality = 0;
	pipelineStateDesc.SampleMask = UINT_MAX;



	pipelineStateDesc.NumRenderTargets = NumRenderTargets;
	for (UINT i = 0; i < NumRenderTargets; i++)
		pipelineStateDesc.RTVFormats[i] = RTVFormats[i];



	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	pipelineStateDesc.pRootSignature = m_RootSignature.Get();


	//ラスタライザステート
	pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineStateDesc.RasterizerState.FrontCounterClockwise = FALSE;
	pipelineStateDesc.RasterizerState.DepthBias = 0;
	pipelineStateDesc.RasterizerState.DepthBiasClamp = 0;
	pipelineStateDesc.RasterizerState.SlopeScaledDepthBias = 0;
	pipelineStateDesc.RasterizerState.DepthClipEnable = FALSE;
	pipelineStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	pipelineStateDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipelineStateDesc.RasterizerState.MultisampleEnable = FALSE;


	//ブレンドステート
	for (int i = 0; i < _countof(pipelineStateDesc.BlendState.RenderTarget); ++i)
	{	
		if (passType == RenderPassType::ForwardTransparent) {
			pipelineStateDesc.BlendState.RenderTarget[i].BlendEnable = TRUE;
			pipelineStateDesc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			pipelineStateDesc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			pipelineStateDesc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			pipelineStateDesc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
			pipelineStateDesc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
			pipelineStateDesc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		} else {
			pipelineStateDesc.BlendState.RenderTarget[i].BlendEnable = TRUE;
			pipelineStateDesc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
			pipelineStateDesc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
			pipelineStateDesc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			pipelineStateDesc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
			pipelineStateDesc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
			pipelineStateDesc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		}
		pipelineStateDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pipelineStateDesc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
		pipelineStateDesc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
		
	}
	pipelineStateDesc.BlendState.AlphaToCoverageEnable = FALSE;
	pipelineStateDesc.BlendState.IndependentBlendEnable = FALSE;


	//デプスステンシルステート
	if (passType == RenderPassType::PostProcess) {
		pipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
		pipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	} else {
		pipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
		pipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		pipelineStateDesc.DepthStencilState.DepthWriteMask = (passType == RenderPassType::ForwardTransparent) ? D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL;
	}
	pipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
	pipelineStateDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	pipelineStateDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	pipelineStateDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipelineStateDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;




	ComPtr<ID3D12PipelineState> pipelineState;
	HRESULT hr = m_Device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(hr));

	// 自動的にHLSLのメタデータをパースして登録する
	if (pipelineState) {
		ShaderMetadata meta = ParseShaderMetadata(ShaderFile);
		m_ShaderMetadataMap[meta.shaderName] = meta;
	}

	return pipelineState;
}








unsigned int RenderManager::CreateShaderResourceView(ID3D12Resource* Resource)
{

	unsigned int index = m_SRVDescriptorPool.front();
	m_SRVDescriptorPool.pop_front();



	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * index;

	if (!Resource)
	{
		return index;
	}

	D3D12_RESOURCE_DESC resDesc = Resource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;


	m_Device->CreateShaderResourceView(Resource, &srvDesc, handle);


	return index;
}


// 指定indexにSRVを作る（プールを消費しない。ブロックスロット用）
void RenderManager::CreateShaderResourceViewAt(unsigned int index, ID3D12Resource* Resource)
{
	if (!Resource) return;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * index;

	D3D12_RESOURCE_DESC resDesc = Resource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

	m_Device->CreateShaderResourceView(Resource, &srvDesc, handle);
}

// 連続8枠を確保し、全スロットをダミーSRVで初期化。先頭indexを返す
unsigned int RenderManager::AllocateMaterialTextureBlock()
{
	if (m_MaterialBlockPool.empty()) {
		assert(false && "Material texture block pool exhausted");
		return MATERIAL_BLOCK_REGION_BASE; // フォールバック
	}
	unsigned int base = m_MaterialBlockPool.front();
	m_MaterialBlockPool.pop_front();

	// 全スロットをダミーで埋める（未割当スロットを安全にサンプルできるように）
	for (unsigned int s = 0; s < MATERIAL_TEX_SLOTS; s++) {
		CreateShaderResourceViewAt(base + s, m_DummyTexture.Get());
	}
	return base;
}

void RenderManager::FreeMaterialTextureBlock(unsigned int blockBase)
{
	m_MaterialBlockPool.push_back(blockBase);
}

// ブロックの指定スロットを実テクスチャのSRVで上書き
void RenderManager::SetMaterialBlockSlot(unsigned int blockBase, unsigned int slot, ID3D12Resource* Resource)
{
	if (slot >= MATERIAL_TEX_SLOTS) return;
	ID3D12Resource* res = Resource ? Resource : m_DummyTexture.Get();
	CreateShaderResourceViewAt(blockBase + slot, res);
}

// space1テーブルをバインド（ブロック先頭ハンドルを丸ごと）
void RenderManager::SetMaterialTextureTable(unsigned int blockBase)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * blockBase;

	m_GraphicsCommandList->SetGraphicsRootDescriptorTable(MATERIAL_TEX_ROOT_PARAM, handle);
}

// ダミーテクスチャ＆既定ブロックを遅延生成（コマンドリスト記録中に呼ぶ）
void RenderManager::EnsureMaterialTextureSetup()
{
	if (m_MaterialTexInitialized) return;
	m_MaterialTexInitialized = true;

	// 1x1 RGBA8 の緑ダミーテクスチャ（検証用に分かりやすい色）
	{
		// DEFAULTヒープのテクスチャ本体（COPY_DEST）
		auto texProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);

		HRESULT hr = m_Device->CreateCommittedResource(&texProp, D3D12_HEAP_FLAG_NONE, &texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_DummyTexture));
		assert(SUCCEEDED(hr));

		// アップロードバッファ（GPUがコピーを消費するまで保持する必要があるためメンバに保存）
		UINT64 reqSize = GetRequiredIntermediateSize(m_DummyTexture.Get(), 0, 1);
		auto upProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(reqSize);
		hr = m_Device->CreateCommittedResource(&upProp, D3D12_HEAP_FLAG_NONE, &upDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_DummyUpload));
		assert(SUCCEEDED(hr));

		// 白（中立。未割当スロットはサンプルしても見た目を変えない）
		const uint8_t pixel[4] = { 255, 255, 255, 255 };
		D3D12_SUBRESOURCE_DATA sub{};
		sub.pData = pixel;
		sub.RowPitch = 4;   // 1px * 4byte
		sub.SlicePitch = 4;

		UpdateSubresources(m_GraphicsCommandList.Get(), m_DummyTexture.Get(), m_DummyUpload.Get(),
			0, 0, 1, &sub);

		auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
			m_DummyTexture.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(1, &trans);
	}

	// 検証用の既定ブロックを確保（全スロット=ダミー緑）
	m_DefaultMaterialBlock = AllocateMaterialTextureBlock();
}

// GPU使用中の可能性があるテクスチャをフェンス通過まで保持してから解放する
void RenderManager::DeferReleaseTexture(std::shared_ptr<Types::TEXTURE> tex)
{
	if (!tex) return;
	// 記録中フレーム＋飛行中フレームをまたいで安全に保持するため +2 のマージン
	m_PendingReleaseTextures.push_back({ std::move(tex), m_FenceValue + 2 });
}


D3D12_GPU_DESCRIPTOR_HANDLE RenderManager::GetShaderResourceViewHandle(unsigned int SRVIndex)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * SRVIndex;

	return handle;
}


void RenderManager::ReleaseShaderResourceView(unsigned int SRVIndex)
{
	m_SRVDescriptorPool.push_front(SRVIndex);
}





unsigned int RenderManager::CreateRenderTargetView(ID3D12Resource* Resource, unsigned int MipLevel)
{
	unsigned int index = m_RTVDescriptorPool.front();
	m_RTVDescriptorPool.pop_front();



	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	handle.ptr += size * index;


	m_Device->CreateRenderTargetView(Resource, nullptr, handle);


	return index;
}


D3D12_CPU_DESCRIPTOR_HANDLE RenderManager::GetRenderTargetViewHandle(unsigned int RTVIndex)
{

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	handle.ptr += size * RTVIndex;

	return handle;
}



void RenderManager::ReleaseRenderTargetView(unsigned int SRVIndex)
{
	m_RTVDescriptorPool.push_front(SRVIndex);
}





std::unique_ptr<RENDER_TARGET> RenderManager::CreateRenderTarget(unsigned int Width, unsigned int Height, DXGI_FORMAT Format, const FLOAT* ClearColor, unsigned int MipLeve)
{

	D3D12_HEAP_PROPERTIES properties{};
	properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 0;
	properties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = Width;
	desc.Height = Height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = MipLeve;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Format = Format;

	D3D12_CLEAR_VALUE clearValue{};
	if (ClearColor) {
		clearValue.Color[0] = ClearColor[0];
		clearValue.Color[1] = ClearColor[1];
		clearValue.Color[2] = ClearColor[2];
		clearValue.Color[3] = ClearColor[3];
	} else {
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;
	}
	clearValue.Format = Format;


	std::unique_ptr<RENDER_TARGET> renderTarget = std::make_unique<RENDER_TARGET>();

	HRESULT hr = m_Device->CreateCommittedResource(&properties,
													D3D12_HEAP_FLAG_NONE,
													&desc,
													D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
													&clearValue,
													IID_PPV_ARGS(&renderTarget->Resource));
	assert(SUCCEEDED(hr));


	renderTarget->SRVIndex = CreateShaderResourceView(renderTarget->Resource.Get());
	renderTarget->SRVHandle = GetShaderResourceViewHandle(renderTarget->SRVIndex);

	renderTarget->RTVIndex = CreateRenderTargetView(renderTarget->Resource.Get());
	renderTarget->RTVHandle = GetRenderTargetViewHandle(renderTarget->RTVIndex);


	return std::move(renderTarget);
}

void Render::RenderManager::CreateRenderTarget() {
    for (UINT i = 0; i < 2; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        HRESULT hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        assert(SUCCEEDED(hr));

        m_Device.Get()->CreateRenderTargetView(backBuffer.Get(), nullptr, m_RenderTargetHandle[i]);
        m_RenderTarget[i] = backBuffer;
    }
}







void RenderManager::SetConstant(CONSTANT_TYPE Type, const void* Constant, unsigned int Size)
{
	assert(m_ConstantBufferIndex[m_RTIndex] < CONSTANT_BUFFER_MAX);


	memcpy(m_ConstantBufferPointer[m_RTIndex] + CONSTANT_BUFFER_SIZE * m_ConstantBufferIndex[m_RTIndex], Constant, Size);


	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * m_ConstantBufferView[m_RTIndex][m_ConstantBufferIndex[m_RTIndex]];

	m_GraphicsCommandList->SetGraphicsRootDescriptorTable((unsigned int)Type, handle);


	m_ConstantBufferIndex[m_RTIndex]++;
}





void RenderManager::SetTexture(TEXTURE_TYPE Type, const TEXTURE* Texture)
{

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * Texture->SRVIndex;

	m_GraphicsCommandList->SetGraphicsRootDescriptorTable((unsigned int)Type, handle);

}



void RenderManager::SetTexture(TEXTURE_TYPE Type, const RENDER_TARGET* Texture)
{

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * Texture->SRVIndex;

	m_GraphicsCommandList->SetGraphicsRootDescriptorTable((unsigned int)Type, handle);

}




std::unique_ptr<VERTEX_BUFFER> RenderManager::CreateVertexBuffer(unsigned int Stride, unsigned int Size)
{
	std::unique_ptr<VERTEX_BUFFER> vertexBuffer = std::make_unique<VERTEX_BUFFER>();

	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buf = CD3DX12_RESOURCE_DESC::Buffer(Stride * Size);
	HRESULT hr = m_Device->CreateCommittedResource(&prop,
													D3D12_HEAP_FLAG_NONE,
													&buf,
													D3D12_RESOURCE_STATE_GENERIC_READ,
													nullptr,
													IID_PPV_ARGS(&vertexBuffer->Resource));
	assert(SUCCEEDED(hr));


	vertexBuffer->Stride = Stride;
	vertexBuffer->Size = Size;


	return std::move(vertexBuffer);
}

void RenderManager::SetVertexBuffer(const VERTEX_BUFFER* VertexBuffer)
{

	D3D12_VERTEX_BUFFER_VIEW vertexView{};
	vertexView.BufferLocation = VertexBuffer->Resource->GetGPUVirtualAddress();
	vertexView.StrideInBytes = VertexBuffer->Stride;
	vertexView.SizeInBytes = VertexBuffer->Stride * VertexBuffer->Size;

	m_GraphicsCommandList->IASetVertexBuffers(0, 1, &vertexView);
}






std::unique_ptr<INDEX_BUFFER> RenderManager::CreateIndexBuffer(unsigned int Size)
{
	std::unique_ptr<INDEX_BUFFER> indexBuffer = std::make_unique<INDEX_BUFFER>();

	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buf = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * Size);
	HRESULT hr = m_Device->CreateCommittedResource(&prop,
		D3D12_HEAP_FLAG_NONE,
		&buf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer->Resource));
	assert(SUCCEEDED(hr));


	indexBuffer->Size = Size;


	return std::move(indexBuffer);
}

void RenderManager::SetIndexBuffer(const INDEX_BUFFER* IndexBuffer)
{

	D3D12_INDEX_BUFFER_VIEW indexView{};
	indexView.BufferLocation = IndexBuffer->Resource->GetGPUVirtualAddress();
	indexView.SizeInBytes = sizeof(unsigned int) * IndexBuffer->Size;
	indexView.Format = DXGI_FORMAT_R32_UINT;
	m_GraphicsCommandList->IASetIndexBuffer(&indexView);

}







void RenderManager::SetPipelineState(const char* PiplineName)
{
	ID3D12PipelineState* pipeline = m_PipelineState[PiplineName].Get();
	assert(pipeline);

	m_GraphicsCommandList->SetPipelineState(pipeline);
}

void RenderManager::RegisterPipelineState(const std::string& name, ComPtr<ID3D12PipelineState> pipelineState) {
	auto it = m_PipelineState.find(name);
	if (it != m_PipelineState.end()) {
		// 次に提出される予定のフェンス値を紐付けて退避させる
		m_PendingReleasePSOs.push_back({ it->second, m_FenceValue + 1 });
	}
	m_PipelineState[name] = pipelineState;
}

void RenderManager::CleanUpRenderTarget() {
	WaitGPU();

	if (m_RenderTarget[0]) {
		m_RenderTarget[0].Reset();
	}
	if (m_RenderTarget[1]) {
		m_RenderTarget[1].Reset();
	}
}

void RenderManager::Resize(unsigned int Width, unsigned int Height) {
	if (Width == 0 || Height == 0) {
		return;
	}
	m_SwapChainResizePending = true;
	m_SwapChainPendingWidth = Width;
	m_SwapChainPendingHeight = Height;
}

void Render::RenderManager::ResizeTarget(RENDER_TARGET_TYPE type, unsigned int width, unsigned int height) {
	if (width == 0 || height == 0) return;

	if (type == RENDER_TARGET_TYPE::GAME_VIEW) {
		m_GameViewResizePending = true;
		m_GameViewPendingWidth = width;
		m_GameViewPendingHeight = height;
	}
	else if (type == RENDER_TARGET_TYPE::SCENE_VIEW) {
		m_SceneViewResizePending = true;
		m_SceneViewPendingWidth = width;
		m_SceneViewPendingHeight = height;
	}
}

void Render::RenderManager::ApplyPendingResizes() {
	if (!m_SwapChainResizePending && !m_GameViewResizePending && !m_SceneViewResizePending) return;

	if (m_SwapChainResizePending) {
		// 1. Close command list first to release all state cache references
		m_GraphicsCommandList->Close();

		// 2. Wait for GPU to ensure we can safely reset and recreate swap chain resources
		WaitGPU();

		// 3. Reset existing render targets to completely release old back buffer references
		m_RenderTarget[0].Reset();
		m_RenderTarget[1].Reset();
		m_DepthBuffer.Reset();

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		HRESULT hr = m_SwapChain->GetDesc1(&desc);
		assert(SUCCEEDED(hr));

		hr = m_SwapChain->ResizeBuffers(0, m_SwapChainPendingWidth, m_SwapChainPendingHeight, desc.Format, desc.Flags);
		assert(SUCCEEDED(hr));

		m_BackBufferWidth = static_cast<int>(m_SwapChainPendingWidth);
		m_BackBufferHeight = static_cast<int>(m_SwapChainPendingHeight);
		m_RTIndex = m_SwapChain->GetCurrentBackBufferIndex();

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

		CreateRenderTarget();

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = 1920; // Fixed 1920 width for G-Buffer depth buffer
		resourceDesc.Height = 1080; // Fixed 1080 height for G-Buffer depth buffer
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
		hr = m_Device->CreateCommittedResource(&prop,
											   D3D12_HEAP_FLAG_NONE,
											   &resourceDesc,
											   D3D12_RESOURCE_STATE_DEPTH_WRITE,
											   &clearValue,
											   IID_PPV_ARGS(&m_DepthBuffer));
		assert(SUCCEEDED(hr));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_DepthBufferHandle = m_DepthBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DepthBufferHandle);

		// Also resize G-Buffers to fixed 1920x1080 resolution
		FLOAT colorClear[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		FLOAT zeroClear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_ColorBuffer.reset();
		m_ColorBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, colorClear);
		m_ColorBuffer->Resource->SetName(L"ColorBuffer");

		m_NormalBuffer.reset();
		m_NormalBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_NormalBuffer->Resource->SetName(L"NormalBuffer");

		m_PositionBuffer.reset();
		m_PositionBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_PositionBuffer->Resource->SetName(L"PositionBuffer");

		m_MaterialBuffer.reset();
		m_MaterialBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_MaterialBuffer->Resource->SetName(L"MaterialBuffer");

		m_EmissionBuffer.reset();
		m_EmissionBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT, zeroClear);
		m_EmissionBuffer->Resource->SetName(L"EmissionBuffer");

		m_LightedColorBuffer.reset();
		m_LightedColorBuffer = CreateRenderTarget(1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_LightedColorBuffer->Resource->SetName(L"LightedColorBuffer");

		m_GameViewTarget.reset();
		m_GameViewTarget = CreateRenderTarget(m_SwapChainPendingWidth, m_SwapChainPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_GameViewTarget->Resource->SetName(L"GameViewTarget");

		m_SceneViewTarget.reset();
		m_SceneViewTarget = CreateRenderTarget(m_SwapChainPendingWidth, m_SwapChainPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
		m_SceneViewTarget->Resource->SetName(L"SceneViewTarget");

		// 4. Force reset all command allocators to ensure they are 100% clean and free of pending graphics commands
		for (int i = 0; i < 2; i++) {
			m_GraphicsCommandAllocator[i]->Reset();
			m_Frame[i] = 0; // Clear recorded GPU fence targets
		}

		// 5. Re-reset the command list under the new back buffer index with clean allocator
		hr = m_GraphicsCommandList->Reset(m_GraphicsCommandAllocator[m_RTIndex].Get(), m_PipelineState["Deferred"].Get());
		assert(SUCCEEDED(hr));

		m_SwapChainResizePending = false;
		m_GameViewResizePending = false;
		m_SceneViewResizePending = false;
	}
	else {
		// Individual view resizing
		// 1. Close command list first to clear active bindings in pipeline cache
		m_GraphicsCommandList->Close();

		// 2. Wait for GPU completion
		WaitGPU();

		if (m_GameViewResizePending) {
			m_GameViewTarget.reset();
			m_GameViewTarget = CreateRenderTarget(m_GameViewPendingWidth, m_GameViewPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_GameViewTarget->Resource->SetName(L"GameViewTarget");
			m_GameViewResizePending = false;
		}

		if (m_SceneViewResizePending) {
			m_SceneViewTarget.reset();
			m_SceneViewTarget = CreateRenderTarget(m_SceneViewPendingWidth, m_SceneViewPendingHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_SceneViewTarget->Resource->SetName(L"SceneViewTarget");
			m_SceneViewResizePending = false;
		}

		// 3. Reset command allocator and command list under current RT index to start recording cleanly
		m_GraphicsCommandAllocator[m_RTIndex]->Reset();
		HRESULT hr = m_GraphicsCommandList->Reset(m_GraphicsCommandAllocator[m_RTIndex].Get(), m_PipelineState["Deferred"].Get());
		assert(SUCCEEDED(hr));
	}
}







TEXTURE::~TEXTURE()
{
	RenderManager::GetInstance()->ReleaseShaderResourceView(SRVIndex);
}


CONSTANT_BUFFER::~CONSTANT_BUFFER()
{
	RenderManager::GetInstance()->ReleaseShaderResourceView(SRVIndex);
}


RENDER_TARGET::~RENDER_TARGET()
{
	RenderManager::GetInstance()->ReleaseShaderResourceView(SRVIndex);
	RenderManager::GetInstance()->ReleaseRenderTargetView(RTVIndex);
}

bool Render::RenderManager::RegisterDynamicPostProcess(const std::string& name, const std::string& shaderFile)
{
	DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
	ComPtr<ID3D12PipelineState> pipeline = CreatePipeline(shaderFile.c_str(), RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	if (!pipeline) {
		return false;
	}
	m_PipelineState[name] = pipeline;
	return true;
}


