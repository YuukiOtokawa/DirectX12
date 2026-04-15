#pragma once
#include "ImGuiWindowController.h"

namespace GUIController {
    namespace EngineWindow {
		using namespace ImGuiControl;

        class DefaultWindowController : public ImGuiWindowController {
        public:
			DefaultWindowController() : ImGuiWindowController("Default Window") {}

            void Initialize() override;
            void Update() override;
            void Draw() override;
            void AfterDraw() override;
            void Finalize() override;
        };

    }
}

