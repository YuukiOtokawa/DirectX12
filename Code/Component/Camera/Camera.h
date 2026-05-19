

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

		REGISTER_COMPONENT(Camera)
	public:
		const XMMATRIX& GetProjectionMatrix() const { return _ProjectionMatrix; }
		const XMMATRIX& GetViewMatrix() const { return _ViewMatrix; }

		Camera();

		void RenderCamera();
		void DrawInspector() override;
	};
}

