#pragma once

#include "Main.h"
#include <memory>

namespace EngineCore::Render {

	class DescriptorAllocator;
	namespace Types { struct TEXTURE; }

	class TextureLoader
	{
	public:
		// DDSファイルを読み込み、GPUリソースを作成してSRVを割り当てる
		static std::unique_ptr<Types::TEXTURE> Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorAllocator& srvAllocator, const char* fileName);
	};

}
