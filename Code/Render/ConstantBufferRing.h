#pragma once

#include "Main.h"
#include "DescriptorAllocator.h"

namespace EngineCore::Render {

	// 定数バッファのフレーム毎リングバッファ（1スロット512byte、1フレーム最大1000スロット、ダブルバッファ）
	class ConstantBufferRing
	{
	public:
		static const unsigned int SLOT_SIZE = 512;
		static const unsigned int SLOT_MAX = 1000;

		void Init(ID3D12Device* device, DescriptorAllocator& srvAllocator);
		void ResetFrame(unsigned int frameIndex);

		// dataをsize分次の空きスロットへコピーし、そのCBVのGPUハンドルを返す
		D3D12_GPU_DESCRIPTOR_HANDLE Write(unsigned int frameIndex, const void* data, unsigned int size);

	private:
		ComPtr<ID3D12Resource>	m_Buffer[2];
		byte*					m_MappedPointer[2] = {};
		unsigned int			m_ViewIndex[2][SLOT_MAX] = {};
		unsigned int			m_WriteIndex[2] = {};
		DescriptorAllocator*	m_SRVAllocator = nullptr;
	};

}
