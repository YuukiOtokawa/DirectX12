#include "Light.h"
#include "../../../ImGui/Code/imgui.h"
#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"
#include "../Camera/Camera.h"
#include "Render/RenderManager.h"

#include <cmath>

using namespace EngineCore::General;
using namespace EngineCore::Render::RenderStructure;

Light* Light::s_ActiveLight = nullptr;

Light::Light() : m_Color(1.0f, 1.0f, 1.0f, 1.0f), m_Exposure(1.0f) {
	m_ViewMatrix = XMMatrixIdentity();
	m_ProjectionMatrix = XMMatrixIdentity();

	m_ShadowOrthoSize = 1.0f;
	m_ShadowFov = 90.0f;
	m_ShadowNear = 0.5f;
	m_ShadowFar = 100.0f;

	if (s_ActiveLight == nullptr) {
		s_ActiveLight = this;
	}
}

void Light::Update() {

}

void Light::Draw() {
	auto transform = GetOwner()->GetComponent<Transform>();
	if (!transform) return;

	auto renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	// Calculate light direction (opposite of transform forward)
	Vector3 forward = transform->GetForward();

	// シャドウ用カメラのUpベクトル（forwardがワールドUpにほぼ平行な場合は代替Upを使う）
	Vector3 up = (std::fabs(forward.y) > 0.99f) ? Vector3(0.0f, 0.0f, 1.0f) : Vector3(0.0f, 1.0f, 0.0f);

	// Directionalは平行光なのでライトの「位置」に意味はなく、「向き」だけを使う。
	// シャドウの箱はアクティブカメラの周囲を覆うように毎フレーム配置する（カメラ追従）。
	// カメラ位置が箱の奥行き[Near, Far]の中央に来るよう、ライト方向の逆へ下げた点を視点にする。
	Vector3 eyePos = transform->Position;
	if (m_LightType == LightType::Directional) {
		auto camera = Camera::GetActiveCamera();
		if (camera && camera->GetOwner()) {
			auto camTransform = camera->GetOwner()->GetComponent<Transform>();
			if (camTransform) {
				float backDistance = (m_ShadowNear + m_ShadowFar) * 0.5f;
				eyePos = camTransform->Position - forward * backDistance;
			}
		}
	}

	XMFLOAT3 posF3 = { eyePos.x, eyePos.y, eyePos.z };
	XMFLOAT3 targetF3 = { eyePos.x + forward.x, eyePos.y + forward.y, eyePos.z + forward.z };
	XMFLOAT3 upF3 = { up.x, up.y, up.z };

	XMVECTOR pos = XMLoadFloat3(&posF3);
	XMVECTOR target = XMLoadFloat3(&targetF3);
	XMVECTOR upVec = XMLoadFloat3(&upF3);
	m_ViewMatrix = XMMatrixLookAtLH(pos, target, upVec);

	// カスケードごとの正射影。幅だけがCASCADE_SCALE倍ずつ広がり、view/near/farは全段共通。
	// （深度レンジが共通なので、シェーダ側の深度バイアスも1つで済む）
	XMMATRIX cascadeProjection[CASCADE_COUNT];
	if (m_LightType == LightType::Directional) {
		for (int i = 0; i < CASCADE_COUNT; i++) {
			float size = m_ShadowOrthoSize * CASCADE_SCALE[i];
			cascadeProjection[i] = XMMatrixOrthographicLH(size, size, m_ShadowNear, m_ShadowFar);
		}
		// シャドウパスでは「今描いているカスケード」の射影を LightProjection として送る
		m_ProjectionMatrix = cascadeProjection[m_CurrentCascade];
	}
	else {
		// Point/Spot: シャドウマップは正方形前提でアスペクト比1.0固定、カスケードなし（全段同じ）
		// Point光源の全方位対応（キューブマップ6面）は未対応
		m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_ShadowFov), 1.0f, m_ShadowNear, m_ShadowFar);
		for (int i = 0; i < CASCADE_COUNT; i++) {
			cascadeProjection[i] = m_ProjectionMatrix;
		}
	}

	ENV_CONSTANT envConstant{};
	envConstant.LightDirection = Vector4(-forward.x, -forward.y, -forward.z, 0.0f);
	envConstant.LightColor = Vector4(m_Color.x * m_Exposure, m_Color.y * m_Exposure, m_Color.z * m_Exposure, m_Color.w);
	XMStoreFloat4x4(&envConstant.LightView, XMMatrixTranspose(m_ViewMatrix));
	XMStoreFloat4x4(&envConstant.LightProjection, XMMatrixTranspose(m_ProjectionMatrix));
	for (int i = 0; i < CASCADE_COUNT; i++) {
		XMStoreFloat4x4(&envConstant.CascadeProjection[i], XMMatrixTranspose(cascadeProjection[i]));
	}
	envConstant.Exposure = m_Exposure;

	renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::ENV, &envConstant, sizeof(envConstant));
}

void Light::Inspector() {
	// Color Picker
	float color[4] = { m_Color.x, m_Color.y, m_Color.z, m_Color.w };
	if (ImGui::ColorEdit4("Light Color", color)) {
		m_Color = Vector4(color[0], color[1], color[2], color[3]);
	}

	// Exposure Slider
	ImGui::SliderFloat("Exposure", &m_Exposure, 0.0f, 10.0f);

	// Light Type
	const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
	int currentType = (int)m_LightType;
	if (ImGui::Combo("Light Type", &currentType, lightTypeNames, _countof(lightTypeNames))) {
		m_LightType = (LightType)currentType;
	}

	if (m_LightType == LightType::Directional) {
		ImGui::DragFloat("Shadow Ortho Size", &m_ShadowOrthoSize, 0.5f, 1.0f, 500.0f, "%.1f");
	}
	else {
		ImGui::DragFloat("Shadow Fov", &m_ShadowFov, 0.5f, 1.0f, 180.0f, "%.1f");
	}
	ImGui::DragFloat("Shadow Near", &m_ShadowNear, 0.05f, 0.01f, 10.0f, "%.2f");
	ImGui::DragFloat("Shadow Far", &m_ShadowFar, 1.0f, 10.0f, 10000.0f, "%.1f");
}
