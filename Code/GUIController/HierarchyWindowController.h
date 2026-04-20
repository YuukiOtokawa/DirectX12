#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
    class HierarchyWindowController :
        public DefaultWindowController {

	public:
		void Draw() override;
    };

}


