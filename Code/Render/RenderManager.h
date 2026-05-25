#pragma once

#include "Main.h"

#include "../Utility/VectorClass.h"

namespace Render {

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
		struct MATERIAL
		{
			Vector4		BaseColor;
			Vector4		EmissionColor;

			float		Metallic;
			float		Specular;
			float		Roughness;
		};



		// 16*4�o�C�g���E///////////////////////
		// 光源データ Lightクラスに移動
		struct ENV_CONSTANT
		{
			Vector4		LightDirection;
			Vector4		LightColor;

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

		// サブセットごとの定数 Rendererクラスに移動
		struct SUBSET_CONSTANT
		{
			MATERIAL Material;
		};

		// テクスチャデータ Textureクラスに移動
		struct TEXTURE
		{
			ComPtr<ID3D12Resource>	Resource;
			unsigned int			SRVIndex;
			~TEXTURE();
		};


		struct CONSTANT_BUFFER
		{
			ComPtr<ID3D12Resource>	Resource;
			unsigned int			SRVIndex;
			~CONSTANT_BUFFER();
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
	using Types::SUBSET_CONSTANT;
	using Types::TEXTURE;
	using Types::CONSTANT_BUFFER;
	using Types::RENDER_TARGET;
	using Types::VERTEX_BUFFER;
	using Types::INDEX_BUFFER;

	class RenderManager
	{

	private:
		static RenderManager* m_Instance;

		HWND								m_WindowHandle;

		bool								m_WindowMode;
		int									m_BackBufferWidth;
		int									m_BackBufferHeight;

		UINT64								m_Frame[2];
		UINT								m_RTIndex;

		ComPtr<IDXGIFactory4>				m_Factory;
		ComPtr<IDXGIAdapter3>				m_Adapter;
		ComPtr<ID3D12Device>				m_Device;
		ComPtr<ID3D12CommandQueue>			m_CommandQueue;
		ComPtr<ID3D12Fence>					m_Fence;
		ComPtr<IDXGISwapChain3>				m_SwapChain;
		ComPtr<ID3D12GraphicsCommandList>	m_GraphicsCommandList;
		ComPtr<ID3D12CommandAllocator>		m_GraphicsCommandAllocator[2];

		HANDLE								m_FenceEvent;

		ComPtr<ID3D12Resource>				m_RenderTarget[2];
		ComPtr<ID3D12DescriptorHeap>		m_RenderTargetDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_RenderTargetHandle[2];

		ComPtr<ID3D12Resource>				m_DepthBuffer;
		ComPtr<ID3D12DescriptorHeap>		m_DepthBufferDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE			m_DepthBufferHandle;

		D3D12_RECT							m_ScissorRect;
		D3D12_VIEWPORT						m_Viewport;

		ComPtr<ID3D12DescriptorHeap>		m_SRVDescriptorHeap;
		std::list<unsigned int>				m_SRVDescriptorPool;
		static const unsigned int			SRV_DESCRIPTOR_MAX = 10000;

		ComPtr<ID3D12DescriptorHeap>		m_RTVDescriptorHeap;
		std::list<unsigned int>				m_RTVDescriptorPool;
		static const unsigned int			RTV_DESCRIPTOR_MAX = 1000;

		static const unsigned int			CONSTANT_BUFFER_SIZE = 512;
		static const unsigned int			CONSTANT_BUFFER_MAX = 1000;
		ComPtr<ID3D12Resource>				m_ConstantBuffer[2];
		byte* m_ConstantBufferPointer[2];
		unsigned int						m_ConstantBufferView[2][CONSTANT_BUFFER_MAX];
		unsigned int						m_ConstantBufferIndex[2];

		ComPtr<ID3D12RootSignature>			m_RootSignature;

		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>	m_PipelineState;
		ComPtr<ID3D12PipelineState> CreatePipeline(const char* VertexShaderFile, const char* PixelShaderFile, const DXGI_FORMAT* RTVFormats, unsigned int NumRenderTargets, bool depthEnable = true);

		std::unique_ptr<VERTEX_BUFFER>		m_VertexBuffer;

		std::unique_ptr<RENDER_TARGET>		m_ColorBuffer;
		std::unique_ptr<RENDER_TARGET>		m_NormalBuffer;

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_ImGuiCPUDescHandles;
		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_ImGuiGPUDescHandles;

		void Init();

	public:

		unsigned int CreateShaderResourceView(ID3D12Resource* Resource);
		D3D12_GPU_DESCRIPTOR_HANDLE GetShaderResourceViewHandle(unsigned int SRVIndex);

		unsigned int CreateRenderTargetView(ID3D12Resource* Resource, unsigned int MipLevel = 0);
		D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle(unsigned int RTVIndex);

		UINT GetShaderResourceViewIndex(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle) {
			D3D12_CPU_DESCRIPTOR_HANDLE startHandle = m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
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

		void DrawBegin();
		void DrawEnd();
		void FrameEnd();

		void DrawScreen();

		void ReleaseShaderResourceView(unsigned int SRVIndex);
		void ReleaseRenderTargetView(unsigned int SRVIndex);

		//�����_�[�^�[�Q�b�g
		std::unique_ptr<RENDER_TARGET> CreateRenderTarget(unsigned int Width, unsigned int Height, DXGI_FORMAT Format, unsigned int MipLevel = 1);

		void CreateRenderTarget();

		void CleanUpRenderTarget();
		void Resize(unsigned int Width, unsigned int Height);

		//�萔�o�b�t�@
		enum class CONSTANT_TYPE
		{
			ENV,
			CAMERA,
			OBJECT,
			SUBSET
		};
		void SetConstant(CONSTANT_TYPE Type, const void* Constant, unsigned int Size);

		//�e�N�X�`��
		enum class TEXTURE_TYPE
		{
			BASE_COLOR = (int)CONSTANT_TYPE::SUBSET + 1,
			NORMAL,
		};
		std::unique_ptr<TEXTURE> LoadTexture(const char* FileName);
		void SetTexture(TEXTURE_TYPE Type, const TEXTURE* Texture);
		void SetTexture(TEXTURE_TYPE Type, const RENDER_TARGET* Texture);

		//���_�o�b�t�@
		std::unique_ptr<VERTEX_BUFFER> CreateVertexBuffer(unsigned int Stride, unsigned int Size);
		void SetVertexBuffer(const VERTEX_BUFFER* VertexBuffer);

		//�C���f�b�N�X�o�b�t�@
		std::unique_ptr<INDEX_BUFFER> CreateIndexBuffer(unsigned int Size);
		void SetIndexBuffer(const INDEX_BUFFER* IndexBuffer);

		IDXGISwapChain3* GetSwapChain() { return m_SwapChain.Get(); }

		void SetPipelineState(const char* PiplineName);

		ID3D12DescriptorHeap* GetSRVDescriptorHeap() { return m_SRVDescriptorHeap.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVDescriptorCPUHandle() {
			return m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_SRVDescriptorPool.pop_front();
		}
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVDescriptorGPUHandle() { return m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(); }
		ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.Get(); }

		RENDER_TARGET* GetColorBuffer() { return m_ColorBuffer.get(); }
		RENDER_TARGET* GetNormalBuffer() { return m_NormalBuffer.get(); }
	};

#pragma endregion RenderManager

}
