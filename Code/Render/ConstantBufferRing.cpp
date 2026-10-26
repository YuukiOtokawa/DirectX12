#include "ConstantBufferRing.h"
#include <cstring>
#include <stdexcept>

namespace EngineCore::Render {

    void ConstantBufferRing::Init(ID3D12Device* device, DescriptorAllocator& srvAllocator) {
        if (!device || m_Device) {
            throw std::logic_error("ConstantBufferRing: invalid or repeated initialization.");
        }
        m_Device = device;
        m_SRVAllocator = &srvAllocator;
    }

    std::unique_ptr<ConstantBufferRing::Page> ConstantBufferRing::CreatePage() {
        // Keep the current shader-visible heap; never replace it mid-frame.
        if (m_SRVAllocator->GetAvailableCount() < SLOTS_PER_PAGE) {
            throw std::runtime_error("ConstantBufferRing: descriptor pool exhausted while growing upload pages.");
        }
        auto page = std::make_unique<Page>();
        D3D12_HEAP_PROPERTIES properties{};
        properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = SLOT_SIZE * SLOTS_PER_PAGE;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.SampleDesc.Count = 1;

        HRESULT hr = m_Device->CreateCommittedResource(&properties,
            D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&page->Buffer));
        if (FAILED(hr)) {
            throw std::runtime_error("ConstantBufferRing: upload page allocation failed, HRESULT=" +
                std::to_string(static_cast<unsigned long>(hr)));
        }
        D3D12_RANGE readRange{0, 0};
        hr = page->Buffer->Map(0, &readRange, reinterpret_cast<void**>(&page->MappedPointer));
        if (FAILED(hr)) {
            throw std::runtime_error("ConstantBufferRing: upload page Map failed, HRESULT=" +
                std::to_string(static_cast<unsigned long>(hr)));
        }

        // Allocation and capacity checks precede descriptor acquisition.
        // Descriptors and pages are retained until engine shutdown.
        for (unsigned int slot = 0; slot < SLOTS_PER_PAGE; ++slot) {
            const unsigned int index = m_SRVAllocator->Allocate();
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
            cbv.BufferLocation = page->Buffer->GetGPUVirtualAddress() + slot * SLOT_SIZE;
            cbv.SizeInBytes = SLOT_SIZE;
            m_Device->CreateConstantBufferView(&cbv, m_SRVAllocator->GetCPUHandle(index));
            page->ViewIndices[slot] = index;
        }
        return page;
    }

    void ConstantBufferRing::ResetFrame(unsigned int frameIndex) {
        if (frameIndex >= 2) throw std::out_of_range("ConstantBufferRing: invalid frame index.");
        m_WriteIndex[frameIndex] = 0;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE ConstantBufferRing::Write(
        unsigned int frameIndex, const void* data, unsigned int size) {
        if (!m_Device) throw std::logic_error("ConstantBufferRing: not initialized.");
        if (frameIndex >= 2) throw std::out_of_range("ConstantBufferRing: invalid frame index.");
        if (!data || size == 0 || size > SLOT_SIZE) {
            throw std::invalid_argument("ConstantBufferRing: constant data must contain 1..512 bytes.");
        }

        const size_t pageIndex = m_WriteIndex[frameIndex] / SLOTS_PER_PAGE;
        const size_t slot = m_WriteIndex[frameIndex] % SLOTS_PER_PAGE;
        auto& pages = m_Pages[frameIndex];
        if (pageIndex == pages.size()) {
            pages.reserve(pages.size() + 1);
            pages.push_back(CreatePage());
        }
        auto& page = *pages[pageIndex];
        std::memcpy(page.MappedPointer + SLOT_SIZE * slot, data, size);
        ++m_WriteIndex[frameIndex];
        return m_SRVAllocator->GetGPUHandle(page.ViewIndices[slot]);
    }
}
