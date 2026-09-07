#include "GraphicsDebugWindowController.h"

#include "../Render/RenderManager.h"

void GUIController::Window::GraphicsDebugWindowController::Draw() {
	ImGui::Text("Color Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetColorBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
    ImGui::Text("Shadow Buffer");
    ImGui::Image((void *)EngineCore::Render::RenderManager::GetInstance()->GetShadowMapBuffer()->SRVHandle.ptr,
                 ImVec2(300.0f, 200.0f));
	ImGui::Text("Normal Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetNormalBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
    ImGui::Text("Position Buffer");
    ImGui::Image((void *)EngineCore::Render::RenderManager::GetInstance()->GetPositionBuffer()->SRVHandle.ptr,
                 ImVec2(300.0f, 200.0f));
	ImGui::Text("Material Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetMaterialBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("Emission Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetEmissionBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("PostProcess Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetPostProcessBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("Lighted Color Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetLightedColorBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));

	DrawBloom();
}

//==================================================
// Bloom
//==================================================

void GUIController::Window::GraphicsDebugWindowController::DrawBloom() {

	auto* renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	ImGui::Separator();
	if (!ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	auto& bloom = renderManager->GetBloomSettings();

	ImGui::Checkbox("Enabled", &bloom.Enabled);

	// RTがR16G16B16A16_FLOATなので1.0超えのHDR値が残っている。閾値は1.0を超えて意味を持つ。
	ImGui::DragFloat("Threshold", &bloom.Threshold, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Knee",      &bloom.Knee,      0.01f, 0.01f,  1.0f);
	ImGui::DragFloat("Intensity", &bloom.Intensity, 0.01f, 0.0f,  5.0f);
	ImGui::DragFloat("Scatter",   &bloom.Scatter,   0.01f, 0.0f,  1.0f);

	// 中間バッファを画面全体に出して確認するためのモード
	const char* debugViewItems[] = {
		"Off (normal)",
		"Prefilter (MipDown[0])",
		"Smallest mip",
		"Final bloom (before composite)",
	};
	ImGui::Combo("Debug View", &bloom.DebugView, debugViewItems, IM_ARRAYSIZE(debugViewItems));

	const unsigned int mipCount = renderManager->GetBloomMipCount();
	ImGui::Text("Mip count: %u", mipCount);

	// ミップチェーンの中身。下り(MipDown)と上り(MipUp)を並べて見る。
	for (unsigned int i = 0; i < mipCount; ++i) {
		auto* down = renderManager->GetBloomMipDown(i);
		auto* up   = renderManager->GetBloomMipUp(i);
		if (!down || !up) continue;

		ImGui::Text("Mip %u  (%.0f x %.0f)", i, down->Size.x, down->Size.y);
		ImGui::Image((void*)down->SRVHandle.ptr, ImVec2(200.0f, 112.0f));
		ImGui::SameLine();
		ImGui::Image((void*)up->SRVHandle.ptr,   ImVec2(200.0f, 112.0f));
	}
}

