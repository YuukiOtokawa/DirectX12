#include "../Manager/Main.h"
#include "RenderManager.h"

#include "D3DX12.h"
#include "../Utility/DDSTextureLoader12.h"
#include "../Utility/ResourcePath.h"
#include "dxgiformat.h"
#include <d3dcompiler.h>
#include <algorithm>
#pragma comment(lib, "d3dcompiler.lib")
using namespace DirectX;

using namespace EngineCore;
using namespace EngineCore::Render;


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




	HRESULT hr;



	m_GraphicsDevice.Init(m_WindowHandle, m_WindowMode, m_BackBufferWidth, m_BackBufferHeight);

	m_Factory = m_GraphicsDevice.GetFactory();
	m_Adapter = m_GraphicsDevice.GetAdapter();
	m_Device = m_GraphicsDevice.GetDevice();
	m_CommandQueue = m_GraphicsDevice.GetCommandQueue();
	m_Fence = m_GraphicsDevice.GetFence();
	m_FenceEvent = m_GraphicsDevice.GetFenceEvent();
	m_SwapChain = m_GraphicsDevice.GetSwapChain();
	m_GraphicsCommandList = m_GraphicsDevice.GetGraphicsCommandList();
	m_GraphicsCommandAllocator[0] = m_GraphicsDevice.GetCommandAllocator(0);
	m_GraphicsCommandAllocator[1] = m_GraphicsDevice.GetCommandAllocator(1);

	m_RTIndex = m_SwapChain->GetCurrentBackBufferIndex();





	m_RenderTargetManager.InitBackBufferAndDepth(m_Device.Get(), m_SwapChain.Get(), m_BackBufferWidth, m_BackBufferHeight);






	// ShaderVisibleDescriptorHeap
	{
		// 汎用プールは予約領域(MATERIAL_BLOCK_REGION_BASE)の手前まで
		m_SRVAllocator.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, SRV_DESCRIPTOR_MAX, MATERIAL_BLOCK_REGION_BASE, true);

		// マテリアルブロック専用領域を8枠刻みでブロックプールへ
		for (unsigned int b = 0; b < MATERIAL_BLOCK_MAX; b++)
			m_MaterialBlockPool.push_back(MATERIAL_BLOCK_REGION_BASE + b * MATERIAL_TEX_SLOTS);
	}

	{
		m_RTVAllocator.Init(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTV_DESCRIPTOR_MAX, RTV_DESCRIPTOR_MAX, false);
	}




	// ConstantBuffer
	m_ConstantBufferRing.Init(m_Device.Get(), m_SRVAllocator);





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

		m_PipelineState["Unlit"] = CreatePipeline(SHADER_DIR "Unlit.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::ForwardOpaque);

	}

	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };

		m_PipelineState["Screen"] = CreatePipeline(SHADER_DIR "Screen.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);

	}


	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		m_PipelineState["Deferred"] = CreatePipeline(SHADER_DIR "Deferred.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	}

	{
        DXGI_FORMAT RTVFormats[] = {DXGI_FORMAT_R16G16B16A16_FLOAT};
		// 深度テスト必須（PostProcessだとDepthEnable=FALSEになり、描画順で深度が上書きされる）
		m_PipelineState["Shadow"] = CreatePipeline(SHADER_DIR "Shadow.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::DeferredOpaque);
	}

	{
		DXGI_FORMAT RTVFormats[] = {
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		};
		m_PipelineState["Geometry"] = CreatePipeline(SHADER_DIR "Geometry.hlsl", RTVFormats, _countof(RTVFormats));
	}

	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		m_PipelineState["PostProcess"] = CreatePipeline(SHADER_DIR "PostProcess.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	}

	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		m_PipelineState["InvertColor"] = CreatePipeline(SHADER_DIR "InvertColor.hlsl", RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	}

	// Bloom : 1ファイル5エントリポイント。エントリ名だけ変えて PSO を作り分ける。
	{
		DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		const char* bloomShader = SHADER_DIR "Bloom.hlsl";

		m_PipelineState["BloomPrefilter"] = CreatePipeline(bloomShader, RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess, "vtx", "pixPrefilter");
		m_PipelineState["BloomBlurH"]     = CreatePipeline(bloomShader, RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess, "vtx", "pixBlurH");
		m_PipelineState["BloomBlurV"]     = CreatePipeline(bloomShader, RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess, "vtx", "pixBlurV");
		m_PipelineState["BloomUpsample"]  = CreatePipeline(bloomShader, RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess, "vtx", "pixUpsample");
		m_PipelineState["BloomComposite"] = CreatePipeline(bloomShader, RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess, "vtx", "pixComposite");
	}

	m_RenderTargetManager.InitGBuffers(m_Device.Get(), m_SRVAllocator, m_RTVAllocator);

	m_EnvTexture = LoadTexture(ASSET_DIR "charolettenbrunn_park_2k.dds");
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


void RenderManager::DrawGeometryBegin() {
    // Descriptor heaps
    ID3D12DescriptorHeap *dh[] = {m_SRVAllocator.GetHeap()};
    m_GraphicsCommandList->SetDescriptorHeaps(_countof(dh), dh);

	// Root signature
    m_GraphicsCommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// マテリアルテクスチャ（space1）の初期化を保証（初回のみ実行）
	EnsureMaterialTextureSetup();

	// 定数バッファリングのリセットはここでは行わない。
	// 1フレームに複数パス（シャドウ→GameView→SceneView→BackBuffer）が
	// 同じコマンドリストへ記録され、GPU実行はFrameEnd後のため、
	// パスごとにリセットすると後のパスが前のパスの定数を上書きしてしまう。
	// リセットはFrameEndでフェンス通過後に1回だけ行う。

	if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::BACK_BUFFER) {
		D3D12_VIEWPORT viewport = m_RenderTargetManager.GetViewport();
		D3D12_RECT scissorRect = m_RenderTargetManager.GetScissorRect();
		m_GraphicsCommandList->RSSetViewports(1, &viewport);
		m_GraphicsCommandList->RSSetScissorRects(1, &scissorRect);

		// Transition back buffer: PRESENT -> RENDER_TARGET
		{
			auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
				m_RenderTargetManager.GetBackBufferResource(m_RTIndex),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(1, &trans);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = m_RenderTargetManager.GetBackBufferHandle(m_RTIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE depthBufferHandle = m_RenderTargetManager.GetDepthBufferHandle();
		m_GraphicsCommandList->OMSetRenderTargets(1, &backBufferHandle, TRUE, &depthBufferHandle);

		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(backBufferHandle, clearColor, 0, nullptr);
		m_GraphicsCommandList->ClearDepthStencilView(depthBufferHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    } else if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::SHADOW_BUFFER) {
        RENDER_TARGET *shadowMapBuffer =
            m_RenderTargetManager.GetShadowMapBuffer();

        // シャドウアトラス専用の深度バッファ（メイン深度は1920x1080でアトラスに足りない）
        D3D12_CPU_DESCRIPTOR_HANDLE depthBufferHandle =
            m_RenderTargetManager.GetShadowDepthBufferHandle();

        // クリア用にアトラス全面のビューポート/シザーを設定
        // （カスケードごとの枠は BeginShadowCascade で改めて設定する）
        const float atlasSize = (float)RenderTargetManager::SHADOW_ATLAS_SIZE;
        D3D12_VIEWPORT atlasViewport{};
        atlasViewport.TopLeftX = 0.0f;
        atlasViewport.TopLeftY = 0.0f;
        atlasViewport.Width = atlasSize;
        atlasViewport.Height = atlasSize;
        atlasViewport.MinDepth = 0.0f;
        atlasViewport.MaxDepth = 1.0f;

        D3D12_RECT atlasScissor{};
        atlasScissor.left = 0;
        atlasScissor.top = 0;
        atlasScissor.right = (LONG)RenderTargetManager::SHADOW_ATLAS_SIZE;
        atlasScissor.bottom = (LONG)RenderTargetManager::SHADOW_ATLAS_SIZE;

        m_GraphicsCommandList->RSSetViewports(1, &atlasViewport);
        m_GraphicsCommandList->RSSetScissorRects(1, &atlasScissor);

        // Transition G-Buffer: PIXEL_SHADER_RESOURCE -> RENDER_TARGET
        {
            D3D12_RESOURCE_BARRIER barriers[1];
            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
                shadowMapBuffer->Resource.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_GraphicsCommandList->ResourceBarrier(1, barriers);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[] = {
            shadowMapBuffer->RTVHandle};
        m_GraphicsCommandList->OMSetRenderTargets(
            _countof(renderTargets), renderTargets, false, &depthBufferHandle);

        FLOAT clearColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
        m_GraphicsCommandList->ClearRenderTargetView(shadowMapBuffer->RTVHandle, clearColor, 0, nullptr);

        m_GraphicsCommandList->ClearDepthStencilView(
            depthBufferHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_IsShadowPass = true;
	}
	else {
		RENDER_TARGET* colorBuffer = m_RenderTargetManager.GetColorBuffer();
		RENDER_TARGET* normalBuffer = m_RenderTargetManager.GetNormalBuffer();
		RENDER_TARGET* positionBuffer = m_RenderTargetManager.GetPositionBuffer();
		RENDER_TARGET* materialBuffer = m_RenderTargetManager.GetMaterialBuffer();
		RENDER_TARGET* emissionBuffer = m_RenderTargetManager.GetEmissionBuffer();
		D3D12_CPU_DESCRIPTOR_HANDLE depthBufferHandle = m_RenderTargetManager.GetDepthBufferHandle();

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

		// Resource barriers for G-Buffer transition
		// Transition G-Buffer: PIXEL_SHADER_RESOURCE -> RENDER_TARGET
		{
			D3D12_RESOURCE_BARRIER barriers[5];
			barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
				colorBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
				normalBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(
				positionBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(
				materialBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(
				emissionBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(5, barriers);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[] =
		{
			colorBuffer->RTVHandle,
			normalBuffer->RTVHandle,
			positionBuffer->RTVHandle,
			materialBuffer->RTVHandle,
			emissionBuffer->RTVHandle
		};
		m_GraphicsCommandList->OMSetRenderTargets(_countof(renderTargets), renderTargets, false, &depthBufferHandle);

		FLOAT clearColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		FLOAT clearNormal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_GraphicsCommandList->ClearRenderTargetView(colorBuffer->RTVHandle, clearColor, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(normalBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(
			positionBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(materialBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearRenderTargetView(emissionBuffer->RTVHandle, clearNormal, 0, nullptr);
		m_GraphicsCommandList->ClearDepthStencilView(depthBufferHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

// カスケードの枠（アトラス2x2グリッドのcascadeIndex番）へ描画先を切り替える。
// DrawGeometryBegin(SHADOW_BUFFER)の後、カスケードごとに呼ぶ。
void RenderManager::BeginShadowCascade(unsigned int cascadeIndex)
{
	RENDER_TARGET* shadowMapBuffer = m_RenderTargetManager.GetShadowMapBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE shadowDepthHandle = m_RenderTargetManager.GetShadowDepthBufferHandle();

	// DrawObjects中にRTVが差し替わる可能性があるので、カスケードごとに再バインドする
	m_GraphicsCommandList->OMSetRenderTargets(1, &shadowMapBuffer->RTVHandle, FALSE, &shadowDepthHandle);

	// 2x2グリッドの cascadeIndex 枠へビューポートを移す
	const float tile = (float)RenderTargetManager::SHADOW_CASCADE_TILE;
	D3D12_VIEWPORT viewport{};
	viewport.TopLeftX = (cascadeIndex % 2) * tile;
	viewport.TopLeftY = (cascadeIndex / 2) * tile;
	viewport.Width = tile;
	viewport.Height = tile;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor{};
	scissor.left   = (LONG)viewport.TopLeftX;
	scissor.top    = (LONG)viewport.TopLeftY;
	scissor.right  = (LONG)(viewport.TopLeftX + tile);
	scissor.bottom = (LONG)(viewport.TopLeftY + tile);

	m_GraphicsCommandList->RSSetViewports(1, &viewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &scissor);
}

//==================================================
// End drawing
//==================================================

void RenderManager::DrawEnd()
{
	if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::BACK_BUFFER) {
		// BackBuffer path: ImGui will render onto it, so we do nothing here.
    } else if (m_RenderTargetManager.GetCurrentTarget() ==
               RENDER_TARGET_TYPE::SHADOW_BUFFER) {
        // Transition back buffer: PRESENT -> PIXEL_SHADER_RESOURCE
        {
            auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
                GetShadowMapBuffer()->Resource.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                );
            m_GraphicsCommandList->ResourceBarrier(1, &trans);
        }

		m_IsShadowPass = false;
    }
	else {
		RENDER_TARGET* lightedColorBuffer = m_RenderTargetManager.GetLightedColorBuffer();
		RENDER_TARGET* target = (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::GAME_VIEW) ? m_RenderTargetManager.GetGameViewTarget() : m_RenderTargetManager.GetSceneViewTarget();
		if (target) {
			// 1) Transition m_LightedColorBuffer: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
			{
				auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
					lightedColorBuffer->Resource.Get(),
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
			D3D12_VIEWPORT targetViewport = m_RenderTargetManager.GetViewport();
			D3D12_RECT targetScissorRect = m_RenderTargetManager.GetScissorRect();
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
			SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, lightedColorBuffer);
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
					lightedColorBuffer->Resource.Get(),
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
	if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}

	RENDER_TARGET* colorBuffer = m_RenderTargetManager.GetColorBuffer();
	RENDER_TARGET* normalBuffer = m_RenderTargetManager.GetNormalBuffer();
	RENDER_TARGET* positionBuffer = m_RenderTargetManager.GetPositionBuffer();
	RENDER_TARGET* materialBuffer = m_RenderTargetManager.GetMaterialBuffer();
	RENDER_TARGET* emissionBuffer = m_RenderTargetManager.GetEmissionBuffer();
	RENDER_TARGET* lightedColorBuffer = m_RenderTargetManager.GetLightedColorBuffer();

	// 1) Transit G-Buffer: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
	{
		D3D12_RESOURCE_BARRIER barriers[5];
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			colorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			normalBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(
			positionBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(
			materialBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(
			emissionBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(5, barriers);
	}

	// 2) Render deferred lighting directly into m_LightedColorBuffer (1920x1080).
	//    Post-process is applied later in ApplyPostProcess(), after the forward pass,
	//    so that both deferred and forward geometry receive the post effects.
	//    m_LightedColorBuffer is already in RENDER_TARGET state (steady-state contract).
	{
		m_GraphicsCommandList->OMSetRenderTargets(1, &lightedColorBuffer->RTVHandle, TRUE, nullptr);

		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(lightedColorBuffer->RTVHandle, clearColor, 0, nullptr);

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
		SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, colorBuffer);
		SetTexture(RenderManager::TEXTURE_TYPE::NORMAL, normalBuffer);
		SetTexture(RenderManager::TEXTURE_TYPE::POSITION, positionBuffer);
		SetTexture(RenderManager::TEXTURE_TYPE::MATERIAL, materialBuffer);
		SetTexture(RenderManager::TEXTURE_TYPE::EMISSION, emissionBuffer);
		SetTexture(RenderManager::TEXTURE_TYPE::ENVIRONMENT, m_EnvTexture.get());
		SetTexture(RenderManager::TEXTURE_TYPE::SHADOW, m_RenderTargetManager.GetShadowMapBuffer());
		DrawScreen();
	}
}

//==================================================
// Apply Post-Process Passes
//==================================================

void RenderManager::ApplyPostProcess()
{
	if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}

	// At this point m_LightedColorBuffer holds the fully composited scene
	// (deferred lighting + forward geometry) and is in RENDER_TARGET state.
	// With no passes registered there is nothing to do: DrawEnd() will copy it as-is.
	if (m_ActivePostProcessPasses.empty()) {
		return;
	}

	RENDER_TARGET* lightedColorBuffer = m_RenderTargetManager.GetLightedColorBuffer();
	RENDER_TARGET* postProcessBuffer = m_RenderTargetManager.GetPostProcessBuffer();

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
	RENDER_TARGET* currentInput = lightedColorBuffer;
	RENDER_TARGET* currentOutput = postProcessBuffer;

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
	if (currentInput == lightedColorBuffer) {
		// Even number of passes: result already lives in m_LightedColorBuffer.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			lightedColorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_GraphicsCommandList->ResourceBarrier(1, &barrier);
	}
	else {
		// Odd number of passes: result lives in m_PostProcessBuffer1; copy it back.
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				lightedColorBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(1, &barrier);
		}

		m_GraphicsCommandList->OMSetRenderTargets(1, &lightedColorBuffer->RTVHandle, TRUE, nullptr);
		FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_GraphicsCommandList->ClearRenderTargetView(lightedColorBuffer->RTVHandle, clearColor, 0, nullptr);

		SetPipelineState("PostProcess");
		SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR, currentInput);
		DrawScreen();
		// m_PostProcessBuffer1 remains in PIXEL_SHADER_RESOURCE (its resting state).
	}
}

//==================================================
// Full-screen pass helper
//==================================================

void RenderManager::DrawFullScreenPass(const char* psoName,
                                       const RENDER_TARGET* input,
                                       const RENDER_TARGET* inputLow,
                                       RENDER_TARGET* output,
                                       const void* constantData,
                                       unsigned int constantSize)
{
	// 全レンダーターゲットの待機状態は PIXEL_SHADER_RESOURCE。描く直前だけ上げて、描き終えたら戻す。
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			output->Resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_GraphicsCommandList->ResourceBarrier(1, &barrier);
	}

	// ビューポートは出力RTの実サイズから引く（ミップごとに変わるので固定値にできない）
	D3D12_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = output->Size.x;
	viewport.Height   = output->Size.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissor{};
	scissor.left   = 0;
	scissor.top    = 0;
	scissor.right  = (LONG)output->Size.x;
	scissor.bottom = (LONG)output->Size.y;

	m_GraphicsCommandList->RSSetViewports(1, &viewport);
	m_GraphicsCommandList->RSSetScissorRects(1, &scissor);

	m_GraphicsCommandList->OMSetRenderTargets(1, &output->RTVHandle, TRUE, nullptr);

	FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_GraphicsCommandList->ClearRenderTargetView(output->RTVHandle, clearColor, 0, nullptr);

	SetPipelineState(psoName);

	if (input)    SetTexture(RenderManager::TEXTURE_TYPE::BASE_COLOR,  input);
	if (inputLow) SetTexture(RenderManager::TEXTURE_TYPE::SCENE_COLOR, inputLow);

	if (constantData && constantSize > 0) {
		SetConstant(RenderManager::CONSTANT_TYPE::SUBSET, constantData, constantSize);
	}

	DrawScreen();

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			output->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(1, &barrier);
	}
}



//==================================================
// Apply Bloom
//==================================================

namespace {

	// Bloom.hlsl の cbuffer BloomConstantBuffer (b3) と同じレイアウト。
	// float4 境界を跨がないよう 32 バイトぴったりに収めてある。
	struct BloomConstants {
		float Threshold;
		float Knee;
		float Intensity;
		float Scatter;
		float InputTexel[2];
		float LowMipTexel[2];
	};

	// テクセルサイズは必ず「入力側」かつ「実際に確保したサイズ」から引く。
	// 奇数解像度は切り捨てで縮むので、2の累乗で割った想定値を使うとズレる。
	inline void SetTexel(float (&dst)[2], const Types::RENDER_TARGET* rt)
	{
		dst[0] = 1.0f / ((rt->Size.x > 0.0f) ? rt->Size.x : 1.0f);
		dst[1] = 1.0f / ((rt->Size.y > 0.0f) ? rt->Size.y : 1.0f);
	}

}

void RenderManager::ApplyBloom()
{
	if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}
	if (!m_BloomSettings.Enabled) {
		return;
	}

	const unsigned int mipCount = m_RenderTargetManager.GetBloomMipCount();
	if (mipCount == 0) {
		return;
	}

	RENDER_TARGET* lightedColorBuffer = m_RenderTargetManager.GetLightedColorBuffer();
	RENDER_TARGET* postProcessBuffer  = m_RenderTargetManager.GetPostProcessBuffer();

	// 入口契約: lightedColorBuffer は RENDER_TARGET 状態で合成済みの絵を持っている。
	// Prefilter と Composite の2回サンプルするので、ここで PSR に落として最後まで維持する。
	// ブルームチェーンは lightedColorBuffer に一度も書き込まないため、元絵の退避コピーは不要。
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			lightedColorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(1, &barrier);
	}

	BloomConstants cb{};
	cb.Threshold = m_BloomSettings.Threshold;
	cb.Knee      = (m_BloomSettings.Knee > 1e-4f) ? m_BloomSettings.Knee : 1e-4f;  // 0除算回避
	cb.Intensity = m_BloomSettings.Intensity;
	cb.Scatter   = m_BloomSettings.Scatter;

	// --- 1) Prefilter : LightedColor -> MipDown[0] (1/2) ---
	{
		RENDER_TARGET* dst = m_RenderTargetManager.GetBloomMipDown(0);
		SetTexel(cb.InputTexel,  lightedColorBuffer);
		SetTexel(cb.LowMipTexel, lightedColorBuffer);
		DrawFullScreenPass("BloomPrefilter", lightedColorBuffer, nullptr, dst, &cb, (unsigned int)sizeof(cb));
	}

	// --- 2) 下り : BlurH で半分に縮めつつ横ブラー、BlurV で縦ブラー ---
	for (unsigned int i = 1; i < mipCount; ++i) {
		RENDER_TARGET* src = m_RenderTargetManager.GetBloomMipDown(i - 1);
		RENDER_TARGET* mid = m_RenderTargetManager.GetBloomMipUp(i);
		RENDER_TARGET* dst = m_RenderTargetManager.GetBloomMipDown(i);

		SetTexel(cb.InputTexel, src);
		DrawFullScreenPass("BloomBlurH", src, nullptr, mid, &cb, (unsigned int)sizeof(cb));

		SetTexel(cb.InputTexel, mid);
		DrawFullScreenPass("BloomBlurV", mid, nullptr, dst, &cb, (unsigned int)sizeof(cb));
	}

	// --- 3) 上り : 低ミップを tent で拡大しながら1段上へ混ぜる ---
	for (int i = (int)mipCount - 2; i >= 0; --i) {
		RENDER_TARGET* high = m_RenderTargetManager.GetBloomMipDown(i);
		RENDER_TARGET* low  = (i == (int)mipCount - 2)
			? m_RenderTargetManager.GetBloomMipDown(i + 1)   // 最下段だけ MipDown を読む
			: m_RenderTargetManager.GetBloomMipUp(i + 1);
		RENDER_TARGET* dst  = m_RenderTargetManager.GetBloomMipUp(i);

		SetTexel(cb.InputTexel,  high);
		SetTexel(cb.LowMipTexel, low);
		DrawFullScreenPass("BloomUpsample", high, low, dst, &cb, (unsigned int)sizeof(cb));
	}

	// mipCount == 1 のときは上りループが回らないので Prefilter の結果がそのまま最終になる
	RENDER_TARGET* bloomResult = (mipCount >= 2)
		? m_RenderTargetManager.GetBloomMipUp(0)
		: m_RenderTargetManager.GetBloomMipDown(0);

	// --- デバッグ表示 : 中間バッファをそのまま画面に出して確認する ---
	if (m_BloomSettings.DebugView != 0) {
		RENDER_TARGET* debugSrc = bloomResult;
		switch (m_BloomSettings.DebugView) {
		case 1:  debugSrc = m_RenderTargetManager.GetBloomMipDown(0); break;             // Prefilter の結果
		case 2:  debugSrc = m_RenderTargetManager.GetBloomMipDown(mipCount - 1); break;  // 最小ミップ
		default: debugSrc = bloomResult; break;                                          // 合成前の最終ブルーム
		}

		// lightedColorBuffer は PSR。中身を捨てて上書きするので、そのまま出力先に使える。
		DrawFullScreenPass("PostProcess", debugSrc, nullptr, lightedColorBuffer);

		// 出口契約に合わせて RENDER_TARGET へ戻す
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				lightedColorBuffer->Resource.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_GraphicsCommandList->ResourceBarrier(1, &barrier);
		}
		return;
	}

	// --- 4) Composite : LightedColor(t0) + bloom(t7) -> PostProcessBuffer1 ---
	// 読みながら同じバッファには書けないので PostProcessBuffer1 を経由する。
	// ApplyPostProcess がこの後 PostProcessBuffer1 を scratch に使うが、時間的に重ならない。
	SetTexel(cb.InputTexel,  lightedColorBuffer);
	SetTexel(cb.LowMipTexel, bloomResult);
	DrawFullScreenPass("BloomComposite", lightedColorBuffer, bloomResult, postProcessBuffer, &cb, (unsigned int)sizeof(cb));

	// --- 5) 結果を LightedColor へ戻す ---
	{
		D3D12_RESOURCE_BARRIER barriers[2];
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			lightedColorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COPY_DEST);
		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			postProcessBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_GraphicsCommandList->ResourceBarrier(2, barriers);
	}

	m_GraphicsCommandList->CopyResource(lightedColorBuffer->Resource.Get(), postProcessBuffer->Resource.Get());

	// 出口契約: LightedColor は RENDER_TARGET、PostProcessBuffer1 は PSR（待機状態）
	{
		D3D12_RESOURCE_BARRIER barriers[2];
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			lightedColorBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			postProcessBuffer->Resource.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_GraphicsCommandList->ResourceBarrier(2, barriers);
	}
}


//==================================================
// Begin Forward Pass
//==================================================

void RenderManager::BeginForwardPass()
{
	if (m_RenderTargetManager.GetCurrentTarget() == RENDER_TARGET_TYPE::BACK_BUFFER) {
		return;
	}

	RENDER_TARGET* lightedColorBuffer = m_RenderTargetManager.GetLightedColorBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE depthBufferHandle = m_RenderTargetManager.GetDepthBufferHandle();

	// Set m_LightedColorBuffer RTV and preserve existing 1920x1080 depth buffer (DSV)
	m_GraphicsCommandList->OMSetRenderTargets(1, &lightedColorBuffer->RTVHandle, TRUE, &depthBufferHandle);

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
			m_RenderTargetManager.GetBackBufferResource(m_RTIndex),
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

		// GPU側で実行が完了したPSO/テクスチャを解放する
		UINT64 completedValue = m_Fence->GetCompletedValue();
		m_PendingReleasePSOs.ReleaseCompleted(completedValue);
		m_PendingReleaseTextures.ReleaseCompleted(completedValue);
	}




	HRESULT hr = m_GraphicsCommandAllocator[m_RTIndex]->Reset();
	assert(SUCCEEDED(hr));

	hr = m_GraphicsCommandList->Reset(m_GraphicsCommandAllocator[m_RTIndex].Get(), m_PipelineState["Deferred"].Get());
	assert(SUCCEEDED(hr));

	// このバッファ(m_RTIndex)を使う前フレームのGPU実行はフェンスで完了済みなので、
	// ここで1回だけ定数バッファリングを巻き戻す（フレーム内の全パスで共有）
	m_ConstantBufferRing.ResetFrame(m_RTIndex);
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
	return TextureLoader::Load(m_Device.Get(), m_GraphicsCommandList.Get(), m_SRVAllocator, FileName);
}






ComPtr<ID3D12PipelineState> RenderManager::CreatePipeline(const char* ShaderFile, const DXGI_FORMAT* RTVFormats, unsigned int NumRenderTargets, RenderPassType passType, const char* vsEntry, const char* psEntry)
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
    bool vsSuccess = compileShader(ShaderFile, vsEntry, "vs_5_1", &vsBlob);
	if (!vsSuccess) return nullptr;
	bool psSuccess = compileShader(ShaderFile, psEntry, "ps_5_1", &psBlob);
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

	unsigned int index = m_SRVAllocator.Allocate();



	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();
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

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_SRVAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();
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
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVAllocator.GetHeap()->GetGPUDescriptorHandleForHeapStart();
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
	m_PendingReleaseTextures.Push(std::move(tex), m_FenceValue + 2);
}


D3D12_GPU_DESCRIPTOR_HANDLE RenderManager::GetShaderResourceViewHandle(unsigned int SRVIndex)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVAllocator.GetHeap()->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * SRVIndex;

	return handle;
}


void RenderManager::ReleaseShaderResourceView(unsigned int SRVIndex)
{
	m_SRVAllocator.Free(SRVIndex);
}





unsigned int RenderManager::CreateRenderTargetView(ID3D12Resource* Resource, unsigned int MipLevel)
{
	unsigned int index = m_RTVAllocator.Allocate();



	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	handle.ptr += size * index;


	m_Device->CreateRenderTargetView(Resource, nullptr, handle);


	return index;
}


D3D12_CPU_DESCRIPTOR_HANDLE RenderManager::GetRenderTargetViewHandle(unsigned int RTVIndex)
{

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	handle.ptr += size * RTVIndex;

	return handle;
}



void RenderManager::ReleaseRenderTargetView(unsigned int SRVIndex)
{
	m_RTVAllocator.Free(SRVIndex);
}





std::unique_ptr<RENDER_TARGET> RenderManager::CreateRenderTarget(unsigned int Width, unsigned int Height, DXGI_FORMAT Format, const FLOAT* ClearColor, unsigned int MipLeve)
{
	return RenderTargetFactory::Create(m_Device.Get(), m_SRVAllocator, m_RTVAllocator, Width, Height, Format, ClearColor, MipLeve);
}

void EngineCore::Render::RenderManager::CreateRenderTarget() {
	m_RenderTargetManager.CreateBackBufferTargets(m_Device.Get(), m_SwapChain.Get());
}







void RenderManager::SetConstant(CONSTANT_TYPE Type, const void* Constant, unsigned int Size)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_ConstantBufferRing.Write(m_RTIndex, Constant, Size);
	m_GraphicsCommandList->SetGraphicsRootDescriptorTable((unsigned int)Type, handle);
}





void RenderManager::SetTexture(TEXTURE_TYPE Type, const TEXTURE* Texture)
{

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVAllocator.GetHeap()->GetGPUDescriptorHandleForHeapStart();
	unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += size * Texture->SRVIndex;

	m_GraphicsCommandList->SetGraphicsRootDescriptorTable((unsigned int)Type, handle);

}



void RenderManager::SetTexture(TEXTURE_TYPE Type, const RENDER_TARGET* Texture)
{

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_SRVAllocator.GetHeap()->GetGPUDescriptorHandleForHeapStart();
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
		m_PendingReleasePSOs.Push(it->second, m_FenceValue + 1);
	}
	m_PipelineState[name] = pipelineState;
}

void RenderManager::CleanUpRenderTarget() {
	WaitGPU();
	m_RenderTargetManager.ResetBackBuffer();
}

void RenderManager::Resize(unsigned int Width, unsigned int Height) {
	m_RenderTargetManager.Resize(Width, Height);
}

void EngineCore::Render::RenderManager::ResizeTarget(RENDER_TARGET_TYPE type, unsigned int width, unsigned int height) {
	m_RenderTargetManager.ResizeTarget(type, width, height);
}

void EngineCore::Render::RenderManager::ApplyPendingResizes() {
	if (!m_RenderTargetManager.HasPendingResize()) return;

	bool wasSwapChainResize = m_RenderTargetManager.HasPendingSwapChainResize();

	// Close command list first to release all state cache references
	m_GraphicsCommandList->Close();

	// Wait for GPU to ensure we can safely reset and recreate swap chain / view resources
	WaitGPU();

	m_RenderTargetManager.ApplyPendingResizes(m_Device.Get(), m_SwapChain.Get(), m_SRVAllocator, m_RTVAllocator, m_BackBufferWidth, m_BackBufferHeight);

	if (wasSwapChainResize) {
		m_RTIndex = m_SwapChain->GetCurrentBackBufferIndex();

		// Force reset all command allocators to ensure they are 100% clean and free of pending graphics commands
		for (int i = 0; i < 2; i++) {
			m_GraphicsCommandAllocator[i]->Reset();
			m_Frame[i] = 0; // Clear recorded GPU fence targets
		}
	}
	else {
		// Reset command allocator under current RT index to start recording cleanly
		m_GraphicsCommandAllocator[m_RTIndex]->Reset();
	}

	// Re-reset the command list under the current back buffer index with clean allocator
	HRESULT hr = m_GraphicsCommandList->Reset(m_GraphicsCommandAllocator[m_RTIndex].Get(), m_PipelineState["Deferred"].Get());
	assert(SUCCEEDED(hr));
}







TEXTURE::~TEXTURE()
{
	RenderManager::GetInstance()->ReleaseShaderResourceView(SRVIndex);
}


RENDER_TARGET::~RENDER_TARGET()
{
	RenderManager::GetInstance()->ReleaseShaderResourceView(SRVIndex);
	RenderManager::GetInstance()->ReleaseRenderTargetView(RTVIndex);
}

bool EngineCore::Render::RenderManager::RegisterDynamicPostProcess(const std::string& name, const std::string& shaderFile)
{
	DXGI_FORMAT RTVFormats[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
	ComPtr<ID3D12PipelineState> pipeline = CreatePipeline(shaderFile.c_str(), RTVFormats, _countof(RTVFormats), RenderPassType::PostProcess);
	if (!pipeline) {
		return false;
	}
	m_PipelineState[name] = pipeline;
	return true;
}


