#include "Menu.h"
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"

#include "utils.h"

namespace F4VRBody {
	extern bool c_staticGripping;

	bool inScopeMenu = false;
	
	ScopeMenuEventHandler scopeMenuEvent;

	bool dynamicGripEnabled = false;

	bool isInScopeMenu() {
		return inScopeMenu;
	}
	 
	EventResult ScopeMenuEventHandler::ReceiveEvent(MenuOpenCloseEvent* a_event, void* dispatcher) {
		const char* name = a_event->menuName.c_str();

		if (!strcmp(name, "ScopeMenu")) {
			if (a_event->isOpen) {
			//	_MESSAGE("scope opened");
				if (!c_staticGripping) {
					c_staticGripping = true;
			//		dynamicGripEnabled = true;
				}
				else {
					dynamicGripEnabled = false;
				}

				inScopeMenu = true;
			}
			else {
				//_MESSAGE("scope closed");
				if (dynamicGripEnabled && c_staticGripping) {
		//			c_staticGripping = false;
				}
				inScopeMenu = false;
			}
		}

		return EventResult::kEvent_Continue;
	}

}