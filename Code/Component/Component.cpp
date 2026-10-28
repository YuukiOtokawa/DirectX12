#include "Component.h"

#include "ImGui/Code/imgui.h"

#include "Code/GameObject/GameObject.h"

namespace EngineCore::General {

	void Component::Start() {}

	void Component::Update() {}

	void Component::DrawInspector() {
		ImGui::Text("%s", GetClassName());
		Inspector();
	}

	void Component::Inspector() {}


}
