

#pragma once
#include "Component.h"

#include "Engine/Source/Utility/VectorClass.h"
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
		~Camera() override {
			if (s_ActiveCamera == this) {
				s_ActiveCamera = nullptr;
			}
		}

		// Component のオーバーライド
		void Draw() override;
		DrawOrder GetDrawOrder() const override { return DrawOrder::Camera; }

		float GetFov() const { return m_Fov; }
		void SetFov(float fov) { m_Fov = fov; }
		float GetNear() const { return m_Near; }
		void SetNear(float nearZ) { m_Near = nearZ; }
		float GetFar() const { return m_Far; }
		void SetFar(float farZ) { m_Far = farZ; }
		Vector3 GetTargetPosition() const { return m_TargetPosition; }
		void SetTargetPosition(const Vector3& target) { m_TargetPosition = target; }

		// アクティブカメラの管理
		static void SetActiveCamera(Camera* camera) { s_ActiveCamera = camera; }
		static Camera* GetActiveCamera() { return s_ActiveCamera; }
	};
}

