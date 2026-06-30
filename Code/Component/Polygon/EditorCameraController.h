#pragma once
#include "Component/Component.h"

namespace EngineCore::General {

	class EditorCameraController : public Component {
		REGISTER_COMPONENT(EditorCameraController)
	private:
		float m_BaseMoveSpeed;
		float m_LookSensitivity;
	public:
		EditorCameraController();
		~EditorCameraController() override = default;

		void Draw() override;
		void Inspector() override {}
	};

}
