#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
    class GraphicsDebugWindowController :
        public DefaultWindowController {


	public:
		GraphicsDebugWindowController() {
			m_WindowName = "Graphics Debug Window";
		}
		void Draw() override;
    };

}


