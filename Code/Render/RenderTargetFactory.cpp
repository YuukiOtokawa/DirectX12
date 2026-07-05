#include "Main.h"
#include "RenderTargetFactory.h"
#include "RenderManager.h"
#include <cassert>

namespace EngineCore::Render {

	std::unique_ptr<Types::RENDER_TARGET> RenderTargetFactory::Create(
		ID3D12Device* device,
		DescriptorAllocator& srvAllocator,
		DescriptorAllocator& rtvAllocator,
		unsigned int width, unsigned int height,
		DXGI_FORMAT format,
		const FLOAT* clearColor,
		unsigned int mipLevel)
	{
		D3D12_HEAP_PROPERTIES properties{};
		properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 0;
		properties.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = width;
		desc.Height = height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = mipLevel;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		desc.Format = format;

		D3D12_CLEAR_VALUE clearValue{};
		if (clearColor) {
			clearValue.Color[0] = clearColor[0];
			clearValue.Color[1] = clearColor[1];
			clearValue.Color[2] = clearColor[2];
			clearValue.Color[3] = clearColor[3];
		} else {
			clearValue.Color[0] = 0.0f;
			clearValue.Color[1] = 0.0f;
			clearValue.Color[2] = 0.0f;
			clearValue.Color[3] = 1.0f;
		}
		clearValue.Format = format;

		std::unique_ptr<Types::RENDER_TARGET> renderTarget = std::make_unique<Types::RENDER_TARGET>();

		HRESULT hr = device->CreateCommittedResource(&properties,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(&renderTarget->Resource));
		assert(SUCCEEDED(hr));

		renderTarget->SRVIndex = srvAllocator.Allocate();
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = mipLevel;
			device->CreateShaderResourceView(renderTarget->Resource.Get(), &srvDesc, srvAllocator.GetCPUHandle(renderTarget->SRVIndex));
		}
		renderTarget->SRVHandle = srvAllocator.GetGPUHandle(renderTarget->SRVIndex);

		renderTarget->RTVIndex = rtvAllocator.Allocate();
		device->CreateRenderTargetView(renderTarget->Resource.Get(), nullptr, rtvAllocator.GetCPUHandle(renderTarget->RTVIndex));
		renderTarget->RTVHandle = rtvAllocator.GetCPUHandle(renderTarget->RTVIndex);

		return renderTarget;
	}

}
