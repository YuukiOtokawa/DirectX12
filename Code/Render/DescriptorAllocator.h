#pragma once

#include "../Manager/Main.h"
#include <list>

namespace EngineCore::Render {

	// SRV/RTVディスクリプタヒープ生成とフリーリスト管理をまとめた汎用アロケータ
	class DescriptorAllocator
	{
	public:
		// heapDescriptorCount: ヒープ全体の枚数
		// poolDescriptorCount: このアロケータのフリーリストで管理する枚数（先頭からheapDescriptorCount以下）
		//   （SRVヒープの一部をマテリアルテクスチャブロック等の別管理領域として予約するため分離）
		void Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int heapDescriptorCount, unsigned int poolDescriptorCount, bool shaderVisible);

		unsigned int Allocate();
		void Free(unsigned int index);

		ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(unsigned int index) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(unsigned int index) const;

	private:
		ComPtr<ID3D12DescriptorHeap>	m_Heap;
		std::list<unsigned int>		m_Pool;
		unsigned int					m_DescriptorSize = 0;
	};

}
