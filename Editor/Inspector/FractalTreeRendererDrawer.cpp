#include "InspectorRegistry.h"

#include "ImGui/Code/imgui.h"
#include "Code/Component/Polygon/FractalTreeRenderer.h"

static void DrawFractalTreeRenderer(EngineCore::General::Component& component) {
	using EngineCore::General::FractalTreeRenderer;
	auto& tree = static_cast<FractalTreeRenderer&>(component);

	// 基底の MeshRenderer の項目（シェーダー・マテリアル等）を先に描く
	EditorUI::InspectorRegistry::Get().DrawAs("MeshRenderer", component);
	ImGui::Separator();
	ImGui::Text("Fractal Tree");

	int depth = tree.GetBranchDepth();
	if (ImGui::SliderInt("Branch Depth", &depth, 0, FractalTreeRenderer::MaxBranchDepth)) {
		tree.SetBranchDepth(depth);
	}
	float angle = tree.GetBranchAngle();
	if (ImGui::SliderFloat("Branch Angle", &angle, 0.0f, 90.0f)) {
		tree.SetBranchAngle(angle);
	}
	float length = tree.GetBranchLength();
	if (ImGui::DragFloat("Branch Length", &length, 0.01f, 0.001f, 1000.0f)) {
		tree.SetBranchLength(length);
	}
	float childScale = tree.GetChildScale();
	if (ImGui::SliderFloat("Child Scale", &childScale, 0.1f, 0.95f)) {
		tree.SetChildScale(childScale);
	}

	ImGui::Text("Branches: %d", (1 << (tree.GetBranchDepth() + 1)) - 1);
}
REGISTER_INSPECTOR(FractalTreeRenderer, DrawFractalTreeRenderer)
