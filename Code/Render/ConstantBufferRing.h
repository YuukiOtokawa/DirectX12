#pragma once

#include "../Manager/Main.h"
#include "DescriptorAllocator.h"

namespace EngineCore::Render {

	// Double-buffered upload pages, retained until shutdown and reused after the GPU fence.
	class ConstantBufferRing
	{
	public:
		static const unsigned int SLOT_SIZE = 512;
		static const unsigned int SLOTS_PER_PAGE = 256;

		void Init(ID3D12Device* device, DescriptorAllocator& srvAllocator);
		// Call only after this frame index's GPU fence has completed.
		void ResetFrame(unsigned int frameIndex);

		// dataをsize分次の空きスロットへコピーし、そのCBVのGPUハンドルを返す
		D3D12_GPU_DESCRIPTOR_HANDLE Write(unsigned int frameIndex, const void* data, unsigned int size);

	private:
		struct Page {
			ComPtr<ID3D12Resource> Buffer;
			byte* MappedPointer = nullptr;
			std::array<unsigned int, SLOTS_PER_PAGE> ViewIndices{};
			~Page() { if (MappedPointer) Buffer->Unmap(0, nullptr); }
		};
		std::unique_ptr<Page> CreatePage();
		ComPtr<ID3D12Device> m_Device;
		std::vector<std::unique_ptr<Page>> m_Pages[2];
		size_t m_WriteIndex[2] = {};
		DescriptorAllocator*	m_SRVAllocator = nullptr;
	};

}
