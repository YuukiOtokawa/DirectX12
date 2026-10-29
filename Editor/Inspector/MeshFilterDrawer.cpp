#include "InspectorRegistry.h"

#include "ThirdParty/ImGui/Code/imgui.h"
#include "Engine/Source/Component/Polygon/MeshFilter.h"
#include "Editor/Utility/FilePicker.h"

static void DrawMeshFilter(EngineCore::General::Component& component) {
	auto& meshFilter = static_cast<EngineCore::General::MeshFilter&>(component);

	if (ImGui::Button("Open Model")) {
		meshFilter.LoadModel(OpenFileDialog());
	}
}
REGISTER_INSPECTOR(MeshFilter, DrawMeshFilter)
