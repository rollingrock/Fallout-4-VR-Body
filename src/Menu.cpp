#include "Menu.h"

#include "utils.h"

namespace frik {
	bool inScopeMenu = false;

	ScopeMenuEventHandler scopeMenuEvent;

	bool isInScopeMenu() {
		return inScopeMenu;
	}

	EventResult ScopeMenuEventHandler::ReceiveEvent(MenuOpenCloseEvent* a_event, void* dispatcher) {
		const char* name = a_event->menuName.c_str();

		if (!_stricmp(name, "ScopeMenu")) {
			if (a_event->isOpen) {
				//	_MESSAGE("scope opened");
				inScopeMenu = true;
			} else {
				//_MESSAGE("scope closed");
				inScopeMenu = false;
			}
		}

		return kEvent_Continue;
	}
}
