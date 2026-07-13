#pragma once

#include "../Manager/Main.h"

#include "../Utility/VectorClass.h"
#include "Material.h"
#include "ShaderMetadata.h"
#include "DescriptorAllocator.h"
#include "DeferredReleaseQueue.h"
#include "ConstantBufferRing.h"
#include "TextureLoader.h"
#include "RenderTargetFactory.h"
#include "GraphicsDevice.h"
#include "RenderTargetType.h"
#include "RenderTargetManager.h"
#include <memory>

namespace EngineCore::Render {

	namespace Types {

		// 頂点データ VertexDataクラスに移動
		struct VERTEX
		{
			Vector3 Position;
			Vector3 Normal;
			Vector2 TexCoord;
			Vector4 Color;
		};

		// マテリアルデータ Materialクラスに移動
		using MATERIAL = EngineCore::Render::MaterialConstant;




		// シャドウカスケード数（Light::CASCADE_COUNT・Common.hlsliの配列数と一致させること）
		static const unsigned int SHADOW_CASCADE_COUNT = 3;

		// 16*4バイト境界///////////////////////
		// 光源データ Lightクラスに移動
		struct ENV_CONSTANT
		{
			Vector4		LightDirection;
			Vector4		LightColor;
            float		Exposure;
            Vector3		Padding; // 16バイト境界に合わせるためのパディング
            XMFLOAT4X4 LightView;
            XMFLOAT4X4 LightProjection;                          // シャドウパス用（今描いているカスケードの射影）
            XMFLOAT4X4 CascadeProjection[SHADOW_CASCADE_COUNT]; // Deferredサンプリング用（全カスケードの射影）
		};

		// カメラデータ Cameraクラスに移動
		struct CAMERA_CONSTANT
		{
			XMFLOAT4X4		View;
			XMFLOAT4X4		Projection;
			Vector4		Position;
		};

		// World行列 Rendererクラスに移動
		struct OBJECT_CONSTANT
		{
			XMFLOAT4X4 World;
		};

		// テクスチャデータ Textureクラスに移動
		struct TEXTURE
		{
			ComPtr<ID3D12Resource>	Resource;
			unsigned int			SRVIndex;
			~TEXTURE();
		};


		struct RENDER_TARGET
		{
			ComPtr<ID3D12Resource>	Resource;
			unsigned int			SRVIndex;
			unsigned int			RTVIndex;
			D3D12_GPU_DESCRIPTOR_HANDLE SRVHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
			~RENDER_TARGET();
		};


		// 頂点バッファデータ MeshFilterクラスに移動
		struct VERTEX_BUFFER
		{
			ComPtr<ID3D12Resource>		Resource;
			unsigned int				Stride;
			unsigned int				Size;
		};

		// インデックスバッファデータ MeshFilterクラスに移動
		struct INDEX_BUFFER
		{
			ComPtr<ID3D12Resource>		Resource;
			unsigned int				Size;
		};
	}

#pragma region RenderManager

	namespace RenderStructure = Types;

	using Types::VERTEX;
	using Types::MATERIAL;
	using Types::ENV_CONSTANT;
	using Types::CAMERA_CONSTANT;
	using Types::OBJECT_CONSTANT;
	using Types::TEXTURE;
	using Types::RENDER_TARGET;
	using Types::VERTEX_BUFFER;
	using Types::INDEX_BUFFER;

	class RenderManager
	{
	public:
		using RENDER_TARGET_TYPE = RenderTargetType;

	private:
		static RenderManager* m_Instance;

		HWND								m_WindowHandle;

		bool								m_WindowMode;
		int									m_BackBufferWidth;
		int									m_BackBufferHeight;

		UINT64								m_Frame[2];
		UINT64								m_FenceValue;
		UINT								m_RTIndex;

		GraphicsDevice						m_GraphicsDevice;

		ComPtr<IDXGIFactory4>				m_Factory;
		ComPtr<IDXGIAdapter3>				m_Adapter;
		ComPtr<ID3D12Device>				m_Device;
		ComPtr<ID3D12CommandQueue>			m_CommandQueue;
		ComPtr<ID3D12Fence>					m_Fence;
		ComPtr<IDXGISwapChain3>				m_SwapChain;
		ComPtr<ID3D12GraphicsCommandList>	m_GraphicsCommandList;
		ComPtr<ID3D12CommandAllocator>		m_GraphicsCommandAllocator[2];

		HANDLE								m_FenceEvent;

		DescriptorAllocator					m_SRVAllocator;
		static const unsigned int			SRV_DESCRIPTOR_MAX = 10000;

		// --- マテリアルテクスチャ（register space1 の連続ディスクリプタテーブル）---
		static const unsigned int			MATERIAL_TEX_SLOTS = 8;        // 1マテリアル最大テクスチャ数（space1 t0..t7）
		static const unsigned int			MATERIAL_BLOCK_MAX = 64;       // 同時に持てるブロック数
		static const unsigned int			MATERIAL_TEX_ROOT_PARAM = 12;  // ルートパラメータ番号（CBV4+SRV8の次）
		static const unsigned int			MATERIAL_BLOCK_REGION_BASE = SRV_DESCRIPTOR_MAX - MATERIAL_TEX_SLOTS * MATERIAL_BLOCK_MAX; // = 9488

		DescriptorAllocator					m_RTVAllocator;
		static const unsigned int			RTV_DESCRIPTOR_MAX = 1000;

		ConstantBufferRing					m_ConstantBufferRing;

		ComPtr<ID3D12RootSignature>			m_RootSignature;

		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>	m_PipelineState;
		std::unordered_map<std::string, ShaderMetadata>					m_ShaderMetadataMap;

		DeferredReleaseQueue<ComPtr<ID3D12PipelineState>>				m_PendingReleasePSOs;

		// GPU使用中の可能性があるテクスチャを、フェンス通過まで保持してから解放する
		DeferredReleaseQueue<std::shared_ptr<Types::TEXTURE>>			m_PendingReleaseTextures;

		std::unique_ptr<VERTEX_BUFFER>		m_VertexBuffer;

		RenderTargetManager					m_RenderTargetManager;

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_ImGuiCPUDescHandles;
		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_ImGuiGPUDescHandles;

		std::unique_ptr<TEXTURE> m_EnvTexture;

		ComPtr<ID3D12Resource>		m_DummyTexture;          // 未割当スロット用の1x1ダミー
		ComPtr<ID3D12Resource>		m_DummyUpload;           // ↑のアップロード用（GPU消費まで保持）
		std::list<unsigned int>		m_MaterialBlockPool;     // 空きブロック先頭indexのリスト
		unsigned int				m_DefaultMaterialBlock = 0; // ステップ1検証用の既定ブロック
		bool						m_MaterialTexInitialized = false;

		bool						m_IsShadowPass = false;
		void Init();

	public:

		unsigned int CreateShaderResourceView(ID3D12Resource* Resource);
		D3D12_GPU_DESCRIPTOR_HANDLE GetShaderResourceViewHandle(unsigned int SRVIndex);

		// --- マテリアルテクスチャ（space1 連続テーブル）---
		// 指定indexにSRVを作る（ブロックスロット用）
		void         CreateShaderResourceViewAt(unsigned int index, ID3D12Resource* Resource);
		// 連続8枠を確保し全スロットをダミーで初期化、先頭indexを返す
		unsigned int AllocateMaterialTextureBlock();
		void         FreeMaterialTextureBlock(unsigned int blockBase);
		// ブロックの指定スロットを実テクスチャのSRVで上書き
		void         SetMaterialBlockSlot(unsigned int blockBase, unsigned int slot, ID3D12Resource* Resource);
		// space1テーブルをバインド
		void         SetMaterialTextureTable(unsigned int blockBase);
		unsigned int GetDefaultMaterialBlock() const { return m_DefaultMaterialBlock; }
		// ダミー＆既定ブロックを遅延生成（コマンドリスト記録中に呼ぶ）
		void         EnsureMaterialTextureSetup();
		// GPU使用中の可能性があるテクスチャをフェンス通過まで保持してから解放する
		void         DeferReleaseTexture(std::shared_ptr<Types::TEXTURE> tex);

		unsigned int CreateRenderTargetView(ID3D12Resource* Resource, unsigned int MipLevel = 0);
		D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle(unsigned int RTVIndex);

		UINT GetShaderResourceViewIndex(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle) {
			D3D12_CPU_DESCRIPTOR_HANDLE startHandle = m_SRVAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();
			unsigned int size = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			return (unsigned int)((CPUHandle.ptr - startHandle.ptr) / size);
		}

		static RenderManager* GetInstance() { return m_Instance; }

		ID3D12Device* GetDevice() { return m_Device.Get(); }
		ID3D12GraphicsCommandList* GetGraphicsCommandList() { return m_GraphicsCommandList.Get(); }

		int GetBackBufferWidth() { return m_BackBufferWidth; }
		int GetBackBufferHeight() { return m_BackBufferHeight; }

		RenderManager();
		~RenderManager();

		void WaitGPU();

		void DrawGeometryBegin();
		// シャドウパス中、アトラス上のカスケード枠へビューポート/RTVを切り替える
		void BeginShadowCascade(unsigned int cascadeIndex);
		void DrawEnd();
		void FrameEnd();

		void DrawScreen();

		void ReleaseShaderResourceView(unsigned int SRVIndex);
		void ReleaseRenderTargetView(unsigned int SRVIndex);

		//レンダーターゲット
		std::unique_ptr<RENDER_TARGET> CreateRenderTarget(unsigned int Width, unsigned int Height, DXGI_FORMAT Format, const FLOAT* ClearColor = nullptr, unsigned int MipLevel = 1);

		void CreateRenderTarget();

		void CleanUpRenderTarget();
		void Resize(unsigned int Width, unsigned int Height);

		void SetCurrentTarget(RENDER_TARGET_TYPE targetType) { m_RenderTargetManager.SetCurrentTarget(targetType); }
		RENDER_TARGET_TYPE GetCurrentTarget() const { return m_RenderTargetManager.GetCurrentTarget(); }

		RENDER_TARGET* GetGameViewTarget() const { return m_RenderTargetManager.GetGameViewTarget(); }
		RENDER_TARGET* GetSceneViewTarget() const { return m_RenderTargetManager.GetSceneViewTarget(); }

		void GetActiveTargetSize(unsigned int& width, unsigned int& height) {
			m_RenderTargetManager.GetActiveTargetSize(width, height, m_BackBufferWidth, m_BackBufferHeight);
		}

		void ResizeTarget(RENDER_TARGET_TYPE type, unsigned int width, unsigned int height);
		void ApplyPendingResizes();

		//定数バッファ
		enum class CONSTANT_TYPE
		{
			ENV,
			CAMERA,
			OBJECT,
			SUBSET
		};
		void SetConstant(CONSTANT_TYPE Type, const void* Constant, unsigned int Size);

		//テクスチャ
		enum class TEXTURE_TYPE
		{
			BASE_COLOR = (int)CONSTANT_TYPE::SUBSET + 1,
			NORMAL,
			POSITION,
			MATERIAL,
			EMISSION,
			ENVIRONMENT,
			SHADOW,
		};
		std::unique_ptr<TEXTURE> LoadTexture(const char* FileName);
		void SetTexture(TEXTURE_TYPE Type, const TEXTURE* Texture);
		void SetTexture(TEXTURE_TYPE Type, const RENDER_TARGET* Texture);

		//頂点バッファ
		std::unique_ptr<VERTEX_BUFFER> CreateVertexBuffer(unsigned int Stride, unsigned int Size);
		void SetVertexBuffer(const VERTEX_BUFFER* VertexBuffer);

		//インデックスバッファ
		std::unique_ptr<INDEX_BUFFER> CreateIndexBuffer(unsigned int Size);
		void SetIndexBuffer(const INDEX_BUFFER* IndexBuffer);

		IDXGISwapChain3* GetSwapChain() { return m_SwapChain.Get(); }

		void SetPipelineState(const char* PiplineName);
		ComPtr<ID3D12PipelineState> CreatePipeline(const char* ShaderFile, const DXGI_FORMAT* RTVFormats, unsigned int NumRenderTargets, RenderPassType passType = RenderPassType::DeferredOpaque);
		void ResolveDeferredLighting();
		void BeginForwardPass();
		void ApplyPostProcess();
		// data/size を渡すと、そのパス描画時に SUBSET(b3) として定数バッファをバインドする
		void AddPostProcessPass(const std::string& psoName, const void* data = nullptr, size_t size = 0) {
			ActivePostProcessPass pass;
			pass.name = psoName;
			if (data && size > 0) {
				pass.propertyBuffer.assign(
					reinterpret_cast<const uint8_t*>(data),
					reinterpret_cast<const uint8_t*>(data) + size);
			}
			m_ActivePostProcessPasses.push_back(std::move(pass));
		}
		void ClearPostProcessPasses() { m_ActivePostProcessPasses.clear(); }
		bool RegisterDynamicPostProcess(const std::string& name, const std::string& shaderFile);
		void RegisterPipelineState(const std::string& name, ComPtr<ID3D12PipelineState> pipelineState);
		const ShaderMetadata* GetShaderMetadata(const std::string& name) const {
			auto it = m_ShaderMetadataMap.find(name);
			if (it != m_ShaderMetadataMap.end()) {
				return &it->second;
			}
			return nullptr;
		}

		ID3D12DescriptorHeap* GetSRVDescriptorHeap() { return m_SRVAllocator.GetHeap(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVDescriptorCPUHandle() {
			return m_SRVAllocator.GetHeap()->GetCPUDescriptorHandleForHeapStart();
		}
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVDescriptorGPUHandle() { return m_SRVAllocator.GetHeap()->GetGPUDescriptorHandleForHeapStart(); }
		ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.Get(); }

		RENDER_TARGET* GetColorBuffer() { return m_RenderTargetManager.GetColorBuffer(); }
		RENDER_TARGET* GetNormalBuffer() { return m_RenderTargetManager.GetNormalBuffer(); }
        RENDER_TARGET *GetPositionBuffer() { return m_RenderTargetManager.GetPositionBuffer(); }
		RENDER_TARGET* GetMaterialBuffer() { return m_RenderTargetManager.GetMaterialBuffer(); }
		RENDER_TARGET* GetEmissionBuffer() { return m_RenderTargetManager.GetEmissionBuffer(); }
		RENDER_TARGET* GetPostProcessBuffer() { return m_RenderTargetManager.GetPostProcessBuffer(); }
        RENDER_TARGET *GetShadowMapBuffer() { return m_RenderTargetManager.GetShadowMapBuffer(); }
		RENDER_TARGET* GetLightedColorBuffer() { return m_RenderTargetManager.GetLightedColorBuffer(); }

		bool IsShadowPass() const { return m_IsShadowPass; }
	private:
		struct ActivePostProcessPass {
			std::string name;
			std::vector<uint8_t> propertyBuffer; // 空ならcbufferバインドなし
		};
		std::vector<ActivePostProcessPass> m_ActivePostProcessPasses;
	};

#pragma endregion RenderManager

}
