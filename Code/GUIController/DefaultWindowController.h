#pragma once
#include "ImGuiWindowController.h"

namespace GUIController::Window {

        class DefaultWindowController : public Gui::ImGuiWindowController {
        public:
			DefaultWindowController() : Gui::ImGuiWindowController("Default Window") {}

            void Initialize() override;
            void Update() override;
            void Draw() override;
            void AfterDraw() override;
            void Finalize() override;
        };

}

