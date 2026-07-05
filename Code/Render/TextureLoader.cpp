#include "Main.h"
#include "TextureLoader.h"
#include "RenderManager.h"
#include "D3DX12.h"
#include "DDSTextureLoader12.h"
#include <cassert>

using namespace DirectX;

namespace EngineCore::Render {

	std::unique_ptr<Types::TEXTURE> TextureLoader::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorAllocator& srvAllocator, const char* fileName)
	{
		std::unique_ptr<Types::TEXTURE> texture = std::make_unique<Types::TEXTURE>();

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresouceData;

		wchar_t wFileName[MAX_PATH];
		size_t size;
		mbstowcs_s(&size, wFileName, fileName, MAX_PATH);

		HRESULT hr = LoadDDSTextureFromFile(device, wFileName, &texture->Resource, ddsData, subresouceData);
		assert(SUCCEEDED(hr));

		texture->Resource->SetName(wFileName);

		D3D12_RESOURCE_DESC desc = texture->Resource->GetDesc();

		unsigned int bpp, block;

		if (desc.Format == DXGI_FORMAT_BC1_UNORM)
		{
			bpp = 4;
			block = 4;
		}
		else if (desc.Format == DXGI_FORMAT_BC6H_UF16)
		{
			bpp = 8;
			block = 4;
		}
		else
		{
			bpp = 32;
			block = 1;
		}

		for (unsigned int a = 0; a < desc.DepthOrArraySize; a++)
		{
			for (unsigned int m = 0; m < desc.MipLevels; m++)
			{
				unsigned int s = a * desc.MipLevels + m;

				unsigned int width = (unsigned int)subresouceData[s].RowPitch * 8 / bpp / block;
				unsigned int height = (unsigned int)subresouceData[s].SlicePitch / (unsigned int)subresouceData[s].RowPitch * block;

				D3D12_BOX box = { 0, 0, 0, width, height, 1 };

				hr = texture->Resource->WriteToSubresource(s, &box, subresouceData[s].pData, (UINT)subresouceData[s].RowPitch, (UINT)subresouceData[s].SlicePitch);
				assert(SUCCEEDED(hr));
			}
		}

		auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
			texture->Resource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdList->ResourceBarrier(1, &trans);

		// SRV作成（RenderManager::CreateShaderResourceViewと同等のロジック）
		unsigned int index = srvAllocator.Allocate();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;

		device->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, srvAllocator.GetCPUHandle(index));

		texture->SRVIndex = index;

		return texture;
	}

}
