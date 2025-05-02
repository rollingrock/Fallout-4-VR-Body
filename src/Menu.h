#pragma once

#include "f4se/GameEvents.h"
#include "f4se/GameMenus.h"

namespace FRIK {


	class ScopeMenuEventHandler : public BSTEventSink<MenuOpenCloseEvent> {
	public:
		virtual EventResult ReceiveEvent(MenuOpenCloseEvent* a_event, void* dispatcher) override;


		static void Register()
		{
			static auto* pHandler = new ScopeMenuEventHandler();
			(*g_ui)->menuOpenCloseEventSource.AddEventSink(pHandler);
		}
	};

	extern ScopeMenuEventHandler scopeMenuEvent;


	bool isInScopeMenu();

}
