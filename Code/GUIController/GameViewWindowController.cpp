#include "GameViewWindowController.h"
#include "Render/RenderManager.h"
#include "../../ImGui/Code/imgui.h"

using namespace GUIController::Window;

void GameViewWindowController::Draw() {
	auto renderManager = Render::RenderManager::GetInstance();
	if (!renderManager) return;

	auto gameViewTarget = renderManager->GetGameViewTarget();
	if (!gameViewTarget) return;

	// Check window size change and resize render target
	ImVec2 size = ImGui::GetContentRegionAvail();
	if (size.x > 0.0f && size.y > 0.0f) {
		D3D12_RESOURCE_DESC desc = gameViewTarget->Resource->GetDesc();
		if (static_cast<UINT>(size.x) != desc.Width || static_cast<UINT>(size.y) != desc.Height) {
			renderManager->ResizeTarget(Render::RenderManager::RENDER_TARGET_TYPE::GAME_VIEW, 
				static_cast<unsigned int>(size.x), static_cast<unsigned int>(size.y));
			
			// Refresh target pointer after resize
			gameViewTarget = renderManager->GetGameViewTarget();
		}
	}

	// Render the target texture to ImGui window
	ImGui::Image((ImTextureID)gameViewTarget->SRVHandle.ptr, size);
}
