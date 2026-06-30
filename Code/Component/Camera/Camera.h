

#pragma once
#include "Component.h"

#include "../../Utility/VectorClass.h"
#include <DirectXMath.h>
using namespace DirectX;

namespace EngineCore::General {

	class Camera : public Component {
		Vector3 m_TargetPosition;
		float m_Fov;

		XMMATRIX m_ProjectionMatrix;
		XMMATRIX m_ViewMatrix;

		float m_Near;
		float m_Far;

		Vector3 m_UpVector;

		Vector3 m_LastPosition;
		Vector3 m_LastRotation;
		Vector3 m_LastTargetPosition;
		float m_TargetDistance;
		bool m_IsInitialized;

		static Camera* s_ActiveCamera;

		REGISTER_COMPONENT(Camera)
	public:
		const XMMATRIX& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const XMMATRIX& GetViewMatrix() const { return m_ViewMatrix; }

		Camera();

		// Component のオーバーライド
		void Draw() override;
		DrawOrder GetDrawOrder() const override { return DrawOrder::Camera; }
		void Inspector() override;

		// アクティブカメラの管理
		static void SetActiveCamera(Camera* camera) { s_ActiveCamera = camera; }
		static Camera* GetActiveCamera() { return s_ActiveCamera; }
	};
}

