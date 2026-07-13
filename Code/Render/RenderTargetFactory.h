#pragma once

#include "../Manager/Main.h"
#include <memory>

namespace EngineCore::Render {

	class DescriptorAllocator;
	namespace Types { struct RENDER_TARGET; }

	class RenderTargetFactory
	{
	public:
		// 指定サイズ・フォーマットのレンダーターゲットを作成し、SRV/RTVを割り当てる
		static std::unique_ptr<Types::RENDER_TARGET> Create(
			ID3D12Device* device,
			DescriptorAllocator& srvAllocator,
			DescriptorAllocator& rtvAllocator,
			unsigned int width, unsigned int height,
			DXGI_FORMAT format,
			const FLOAT* clearColor = nullptr,
			unsigned int mipLevel = 1);
	};

}
