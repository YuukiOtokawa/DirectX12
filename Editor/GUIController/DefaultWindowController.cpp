#include "DefaultWindowController.h"

using namespace GUIController::Window;

void DefaultWindowController::Initialize() {}

void DefaultWindowController::Update() {}

void DefaultWindowController::Draw() {
	::ImGui::Text("This is Default Window");
}

void GUIController::Window::DefaultWindowController::AfterDraw() {
	::ImGui::End();
}

void DefaultWindowController::Finalize() {}
