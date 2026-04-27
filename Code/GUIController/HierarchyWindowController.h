#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
    class HierarchyWindowController :
        public DefaultWindowController {


	public:
		HierarchyWindowController() {
			_WindowName = "Hierarchy Window";
		}
		void Draw() override;
    };

}


