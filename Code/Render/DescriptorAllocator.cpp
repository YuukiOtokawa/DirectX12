#include "DescriptorAllocator.h"

namespace EngineCore::Render {

	void DescriptorAllocator::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int heapDescriptorCount, unsigned int poolDescriptorCount, bool shaderVisible)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = heapDescriptorCount;
		desc.Type = type;
		desc.NodeMask = 0;

		device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap));

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

		for (unsigned int i = 0; i < poolDescriptorCount; i++)
			m_Pool.push_back(i);
	}

	unsigned int DescriptorAllocator::Allocate()
	{
		unsigned int index = m_Pool.front();
		m_Pool.pop_front();
		return index;
	}

	void DescriptorAllocator::Free(unsigned int index)
	{
		m_Pool.push_front(index);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetCPUHandle(unsigned int index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_Heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(m_DescriptorSize) * index;
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetGPUHandle(unsigned int index) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = m_Heap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<UINT64>(m_DescriptorSize) * index;
		return handle;
	}

}
