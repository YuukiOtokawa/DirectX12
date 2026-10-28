#include "InspectorRegistry.h"

#include "ImGui/Code/imgui.h"
#include "Code/Component/Polygon/MeshFilter.h"
#include "Code/Utility/FilePicker.h"

static void DrawMeshFilter(EngineCore::General::Component& component) {
	auto& meshFilter = static_cast<EngineCore::General::MeshFilter&>(component);

	if (ImGui::Button("Open Model")) {
		meshFilter.LoadModel(OpenFileDialog());
	}
}
REGISTER_INSPECTOR(MeshFilter, DrawMeshFilter)
