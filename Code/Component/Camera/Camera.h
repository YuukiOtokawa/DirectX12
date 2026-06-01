

#pragma once
#include "Component.h"

#include "../../Utility/VectorClass.h"
#include <DirectXMath.h>
using namespace DirectX;

namespace EngineCore::General {

	class Camera : public Component {
		Vector3 _TargetPosition;
		float _Fov;

		XMMATRIX _ProjectionMatrix;
		XMMATRIX _ViewMatrix;

		float _Near;
		float _Far;

		Vector3 _UpVector;

		static Camera* s_ActiveCamera;

		REGISTER_COMPONENT(Camera)
	public:
		const XMMATRIX& GetProjectionMatrix() const { return _ProjectionMatrix; }
		const XMMATRIX& GetViewMatrix() const { return _ViewMatrix; }

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

