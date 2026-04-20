#pragma once
#include "DefaultWindowController.h"

namespace GUIController::EngineWindow::ImGuiControl {
    class HierarchyWindowController :
        public DefaultWindowController {

	public:
		void Draw() override;
    };

}


