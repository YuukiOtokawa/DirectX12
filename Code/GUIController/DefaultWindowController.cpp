#include "DefaultWindowController.h"

using namespace GUIController::EngineWindow;

void DefaultWindowController::Initialize() {}

void DefaultWindowController::Update() {}

void DefaultWindowController::Draw() {
	ImGui::Text("This is Default Window");
}

void GUIController::EngineWindow::DefaultWindowController::AfterDraw() {
	ImGui::End();
}

void DefaultWindowController::Finalize() {}
