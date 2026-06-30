#include "GameViewWindowController.h"
#include "Render/RenderManager.h"
#include "../../ImGui/Code/imgui.h"

using namespace GUIController::Window;

void GameViewWindowController::Draw() {
	auto renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	auto gameViewTarget = renderManager->GetGameViewTarget();
	if (!gameViewTarget) return;

	// Get aspect ratio of the original render texture (G-Buffer)
	float aspect = 16.0f / 9.0f;
	auto colorBuffer = renderManager->GetColorBuffer();
	if (colorBuffer && colorBuffer->Resource) {
		D3D12_RESOURCE_DESC desc = colorBuffer->Resource->GetDesc();
		if (desc.Height > 0) {
			aspect = static_cast<float>(desc.Width) / static_cast<float>(desc.Height);
		}
	}

	ImVec2 availSize = ImGui::GetContentRegionAvail();
	if (availSize.x > 0.0f && availSize.y > 0.0f) {
		// Calculate draw size maintaining the aspect ratio
		ImVec2 drawSize;
		float windowAspect = availSize.x / availSize.y;
		if (windowAspect > aspect) {
			drawSize.y = availSize.y;
			drawSize.x = availSize.y * aspect;
		} else {
			drawSize.x = availSize.x;
			drawSize.y = availSize.x / aspect;
		}

		// Ensure width and height are at least 1
		UINT targetWidth = static_cast<UINT>(drawSize.x);
		UINT targetHeight = static_cast<UINT>(drawSize.y);
		if (targetWidth < 1) targetWidth = 1;
		if (targetHeight < 1) targetHeight = 1;

		// Check window size change and resize render target
		D3D12_RESOURCE_DESC desc = gameViewTarget->Resource->GetDesc();
		if (targetWidth != desc.Width || targetHeight != desc.Height) {
			renderManager->ResizeTarget(EngineCore::Render::RenderManager::RENDER_TARGET_TYPE::GAME_VIEW, 
				targetWidth, targetHeight);
			
			// Refresh target pointer after resize
			gameViewTarget = renderManager->GetGameViewTarget();
		}

		// Centering the image in the available region
		ImVec2 cursorPos = ImGui::GetCursorPos();
		cursorPos.x += (availSize.x - drawSize.x) * 0.5f;
		cursorPos.y += (availSize.y - drawSize.y) * 0.5f;
		ImGui::SetCursorPos(cursorPos);

		// Render the target texture to ImGui window
		ImGui::Image((ImTextureID)gameViewTarget->SRVHandle.ptr, drawSize);
	}
}
