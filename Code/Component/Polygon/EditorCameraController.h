#pragma once
#include "Component/Component.h"

namespace EngineCore::General {

	class EditorCameraController : public Component {
		REGISTER_COMPONENT(EditorCameraController)
	private:
		float _BaseMoveSpeed;
		float _LookSensitivity;
	public:
		EditorCameraController();
		~EditorCameraController() override = default;

		void Draw() override;
		void Inspector() override {}
	};

}
