#include "FractalTreeRenderer.h"
#include "MeshFilter.h"
#include "../Transform/Transform.h"
#include "../../GameObject/GameObject.h"
#include "../../../ImGui/Code/imgui.h"
#include <cmath>

using namespace DirectX;
using namespace EngineCore::General;

void FractalTreeRenderer::Draw() {
	auto* owner = GetOwner();
	if (!owner) return;

	auto* transform = owner->GetComponent<Transform>();
	auto* meshFilter = owner->GetComponent<MeshFilter>();
	if (!transform || !meshFilter ||
		!meshFilter->GetVertexBuffer() || !meshFilter->GetVertexData()) return;

	auto* renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager || (renderManager->IsShadowPass() && !GetCastShadows())) return;

	const auto& vertices = meshFilter->GetVertexData()->GetVertices();
	if (vertices.empty()) return;
	float minY = vertices.front().Position.y;
	float maxY = minY;
	for (const auto& vertex : vertices) {
		if (!std::isfinite(vertex.Position.y)) return;
		minY = (std::min)(minY, vertex.Position.y);
		maxY = (std::max)(maxY, vertex.Position.y);
	}
	const float sourceLength = maxY - minY;
	if (!std::isfinite(sourceLength) || sourceLength <= 0.000001f) return;

	// Move the mesh base to Y=0 and stretch its tip to Branch Length.
	// This correction affects only the mesh, not the children's coordinate frames.
	const XMMATRIX meshToBranch =
		XMMatrixTranslation(0.0f, -minY, 0.0f) *
		XMMatrixScaling(1.0f, m_BranchLength / sourceLength, 1.0f);

	const auto scale = transform->GetScale();
	const auto rotation = transform->GetRotation();
	const auto position = transform->GetPosition();
	const XMMATRIX rootWorld =
		XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
		XMMatrixTranslation(position.x, position.y, position.z);

	DrawBranch(rootWorld, meshToBranch, m_BranchDepth);
}

void FractalTreeRenderer::DrawBranch(const XMMATRIX& world,
	const XMMATRIX& meshToBranch, int remainingDepth) {
	// Recursion issues consecutive draws without switching material or render pass.
	DrawMesh(meshToBranch * world, remainingDepth == m_BranchDepth);
	if (remainingDepth <= 0) return;

	const float angle = XMConvertToRadians(m_BranchAngleDegrees);
	const XMMATRIX childScale =
		XMMatrixScaling(m_ChildScale, m_ChildScale, m_ChildScale);
	const XMMATRIX toParentTip =
		XMMatrixTranslation(0.0f, m_BranchLength, 0.0f);

	// Scale/rotate the child around its root, then attach it to the parent's tip.
	const XMMATRIX leftWorld =
		childScale * XMMatrixRotationZ(angle) * toParentTip * world;
	const XMMATRIX rightWorld =
		childScale * XMMatrixRotationZ(-angle) * toParentTip * world;

	DrawBranch(leftWorld, meshToBranch, remainingDepth - 1);
	DrawBranch(rightWorld, meshToBranch, remainingDepth - 1);
}

void FractalTreeRenderer::Inspector() {
	MeshRenderer::Inspector();
	ImGui::Separator();
	ImGui::Text("Fractal Tree");

	if (ImGui::SliderInt("Branch Depth", &m_BranchDepth, 0, MaxBranchDepth)) {
		SetBranchDepth(m_BranchDepth);
	}
	if (ImGui::SliderFloat("Branch Angle", &m_BranchAngleDegrees, 0.0f, 90.0f)) {
		SetBranchAngle(m_BranchAngleDegrees);
	}
	if (ImGui::DragFloat("Branch Length", &m_BranchLength, 0.01f, 0.001f, 1000.0f)) {
		m_BranchLength = std::clamp(m_BranchLength, 0.001f, 1000.0f);
	}
	if (ImGui::SliderFloat("Child Scale", &m_ChildScale, 0.1f, 0.95f)) {
		m_ChildScale = std::clamp(m_ChildScale, 0.1f, 0.95f);
	}

	ImGui::Text("Branches: %d", (1 << (m_BranchDepth + 1)) - 1);
}
