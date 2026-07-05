#include "ConstantBufferRing.h"
#include <cassert>
#include <cstring>

namespace EngineCore::Render {

	void ConstantBufferRing::Init(ID3D12Device* device, DescriptorAllocator& srvAllocator)
	{
		m_SRVAllocator = &srvAllocator;

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
				desc.Width = SLOT_SIZE * SLOT_MAX;

				HRESULT hr = device->CreateCommittedResource(&properties,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_Buffer[i]));
				assert(SUCCEEDED(hr));
			}

			HRESULT hr = m_Buffer[i]->Map(0, nullptr, (void**)&m_MappedPointer[i]);
			assert(SUCCEEDED(hr));

			for (unsigned int j = 0; j < SLOT_MAX; j++)
			{
				unsigned int index = srvAllocator.Allocate();

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = m_Buffer[i]->GetGPUVirtualAddress() + j * SLOT_SIZE;
				cbvDesc.SizeInBytes = SLOT_SIZE;

				device->CreateConstantBufferView(&cbvDesc, srvAllocator.GetCPUHandle(index));

				m_ViewIndex[i][j] = index;
			}

			m_WriteIndex[i] = 0;
		}
	}

	void ConstantBufferRing::ResetFrame(unsigned int frameIndex)
	{
		m_WriteIndex[frameIndex] = 0;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE ConstantBufferRing::Write(unsigned int frameIndex, const void* data, unsigned int size)
	{
		assert(m_WriteIndex[frameIndex] < SLOT_MAX);

		unsigned int slot = m_WriteIndex[frameIndex]++;
		memcpy(m_MappedPointer[frameIndex] + SLOT_SIZE * slot, data, size);

		return m_SRVAllocator->GetGPUHandle(m_ViewIndex[frameIndex][slot]);
	}

}
