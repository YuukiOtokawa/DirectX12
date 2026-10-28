#include "InspectorRegistry.h"

#include "ImGui/Code/imgui.h"
#include "Code/Component/Component.h"

namespace EditorUI {

	InspectorRegistry& InspectorRegistry::Get() {
		static InspectorRegistry instance;
		return instance;
	}

	void InspectorRegistry::Register(const std::string& typeName, ComponentDrawer drawer) {
		m_Drawers[typeName] = std::move(drawer);
	}

	void InspectorRegistry::Draw(EngineCore::General::Component& component) {
		const char* typeName = component.GetTypeName();
		ImGui::Text("%s", typeName);

		if (!DrawAs(typeName, component)) {
			// Drawer へ移行していないコンポーネントは従来の Inspector() で描く
			component.Inspector();
		}
	}

	bool InspectorRegistry::DrawAs(const std::string& typeName, EngineCore::General::Component& component) {
		auto it = m_Drawers.find(typeName);
		if (it == m_Drawers.end()) {
			return false;
		}
		it->second(component);
		return true;
	}
}
