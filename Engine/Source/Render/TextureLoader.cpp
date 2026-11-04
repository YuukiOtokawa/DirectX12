#include "Engine/Source/Manager/Main.h"
#include "TextureLoader.h"
#include "RenderManager.h"
#include "D3DX12.h"
#include "Engine/Source/Utility/DDSTextureLoader12.h"
#include "Engine/Source/Utility/Log.h"
#include "ThirdParty/DirectXTex/DirectXTex.h"
#include <algorithm>
#include <cctype>
#include <string>

using namespace DirectX;

namespace EngineCore::Render {

	// ブロック圧縮フォーマットの bits-per-pixel / ブロックサイズ(px)を判定
	// （BC1系=4bpp, BC2/3/5/6H/7系=8bpp、いずれも4x4ブロック。非圧縮は32bpp/block=1として扱う）
	static void GetBlockCompressionInfo(DXGI_FORMAT format, unsigned int& bpp, unsigned int& block)
	{
		switch (format)
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			bpp = 4;
			block = 4;
			break;
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			bpp = 8;
			block = 4;
			break;
		default:
			bpp = 32;
			block = 1;
			break;
		}
	}

	// 拡張子を小文字で返す。std::filesystem::path は変換できない文字で例外を投げるので使わない
	static std::string GetLowerExtension(const char* fileName)
	{
		const std::string path = fileName;
		const size_t dot = path.find_last_of('.');
		if (dot == std::string::npos) return "";
		std::string extension = path.substr(dot);
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return extension;
	}

	// DDS 以外（PNG / JPG / BMP など WIC が読める形式と TGA）を読み、RGBA8 のリソースとミップを作る。
	// subresouceData は image のメモリを指すので、image は書き込みが終わるまで保持する。
	// DDS と同じく色はそのままの値で扱う（sRGB のフォーマットにしない）
	static HRESULT LoadImageTextureFromFile(ID3D12Device* device, const wchar_t* fileName, bool isTga, ID3D12Resource** resource, ScratchImage& image, std::vector<D3D12_SUBRESOURCE_DATA>& subresouceData)
	{
		// WIC は COM を使う。DirectXTex が WIC のファクトリを持ち続けるので、解除しない
		static const HRESULT s_ComResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		(void)s_ComResult; // 既に別のモードで初期化済みでも WIC は使える

		ScratchImage loaded;
		HRESULT hr = isTga
			? LoadFromTGAFile(fileName, TGA_FLAGS_IGNORE_SRGB, nullptr, loaded)
			: LoadFromWICFile(fileName, WIC_FLAGS_IGNORE_SRGB, nullptr, loaded);
		if (FAILED(hr)) return hr;

		if (loaded.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM) {
			ScratchImage converted;
			hr = Convert(*loaded.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
			if (FAILED(hr)) return hr;
			loaded = std::move(converted);
		}

		const TexMetadata& source = loaded.GetMetadata();
		if (source.width == 1 && source.height == 1) {
			image = std::move(loaded); // 1x1 はミップを作れない
		}
		else {
			hr = GenerateMipMaps(*loaded.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, image);
			if (FAILED(hr)) return hr;
		}

		const TexMetadata& metadata = image.GetMetadata();
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, static_cast<UINT>(metadata.height), 1, static_cast<UINT16>(metadata.mipLevels));

		// DDS の読み込みと同じく、CPU から WriteToSubresource で書き込めるヒープに置く
		auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
		hr = device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(resource));
		if (FAILED(hr)) return hr;

		for (size_t mip = 0; mip < metadata.mipLevels; mip++) {
			const Image* level = image.GetImage(mip, 0, 0);
			subresouceData.push_back({ level->pixels, static_cast<LONG_PTR>(level->rowPitch), static_cast<LONG_PTR>(level->slicePitch) });
		}
		return S_OK;
	}

	std::unique_ptr<Types::TEXTURE> TextureLoader::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorAllocator& srvAllocator, const char* fileName)
	{
		if (srvAllocator.GetAvailableCount() == 0) {
			Log::Write(Log::Level::Error, std::string("Failed to load texture (no free SRV): ") + fileName);
			return nullptr;
		}

		std::unique_ptr<Types::TEXTURE> texture = std::make_unique<Types::TEXTURE>();

		// subresouceData が指すメモリ。形式に応じてどちらかを使う
		std::unique_ptr<uint8_t[]> ddsData;
		ScratchImage image;
		std::vector<D3D12_SUBRESOURCE_DATA> subresouceData;

		// 長すぎるパスは切り詰める。その場合はファイルが見つからない失敗として扱われる
		wchar_t wFileName[MAX_PATH];
		size_t size;
		mbstowcs_s(&size, wFileName, fileName, _TRUNCATE);

		const std::string extension = GetLowerExtension(fileName);
		HRESULT hr = (extension == ".dds")
			? LoadDDSTextureFromFile(device, wFileName, &texture->Resource, ddsData, subresouceData)
			: LoadImageTextureFromFile(device, wFileName, extension == ".tga", &texture->Resource, image, subresouceData);
		if (FAILED(hr)) {
			Log::Write(Log::Level::Error, std::string("Failed to load texture: ") + fileName + " (" + Log::FormatHresult(hr) + ")");
			return nullptr;
		}

		texture->Resource->SetName(wFileName);

		D3D12_RESOURCE_DESC desc = texture->Resource->GetDesc();

		// SRV は TEXTURE2D で作り、下の書き込みも 2D 前提なので、ボリュームなどは読まない
		if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
			Log::Write(Log::Level::Error, std::string("Failed to load texture (not a 2D texture): ") + fileName);
			return nullptr;
		}

		unsigned int bpp, block;
		GetBlockCompressionInfo(desc.Format, bpp, block);

		for (unsigned int a = 0; a < desc.DepthOrArraySize; a++)
		{
			for (unsigned int m = 0; m < desc.MipLevels; m++)
			{
				unsigned int s = a * desc.MipLevels + m;

				unsigned int width = (unsigned int)subresouceData[s].RowPitch * 8 / bpp / block;
				unsigned int height = (unsigned int)subresouceData[s].SlicePitch / (unsigned int)subresouceData[s].RowPitch * block;

				D3D12_BOX box = { 0, 0, 0, width, height, 1 };

				hr = texture->Resource->WriteToSubresource(s, &box, subresouceData[s].pData, (UINT)subresouceData[s].RowPitch, (UINT)subresouceData[s].SlicePitch);
				if (FAILED(hr)) {
					Log::Write(Log::Level::Error, std::string("Failed to write texture: ") + fileName + " (" + Log::FormatHresult(hr) + ")");
					return nullptr;
				}
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
