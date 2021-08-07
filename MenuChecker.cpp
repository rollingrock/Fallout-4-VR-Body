#include "MenuChecker.h"


namespace SmoothMovementVR
{
	std::vector<std::string> gameStoppingMenusNoDialogue{
		"BarterMenu",
		"Book Menu",
		"Console",
		"Native UI Menu",
		"ContainerMenu",
		"Crafting Menu",
		"Credits Menu",
		"Cursor Menu",
		"Debug Text Menu",
		"FavoritesMenu",
		"GiftMenu",
		"InventoryMenu",
		"Journal Menu",
		"Kinect Menu",
		"LoadingMenu",
		"Lockpicking Menu",
		"MagicMenu",
		"MainMenu",
		"PipboyMenu",
		"LevelUpMenu",
		"PauseMenu",
		"MapMarkerText3D",
		"MapMenu",
		"MessageBoxMenu",
		"Mist Menu",
		"Quantity Menu",
		"RaceSex Menu",
		"Sleep/Wait Menu",
		"StatsMenuSkillRing",
		"StatsMenuPerks",
		"Training Menu",
		"Tutorial Menu",
		"TweenMenu"
	};

	std::vector<std::string> gameStoppingMenus{
		"BarterMenu",
		"Book Menu",
		"Console",
		"Native UI Menu",
		"ContainerMenu",
		"Dialogue Menu",
		"Crafting Menu",
		"Credits Menu",
		"Cursor Menu",
		"Debug Text Menu",
		"FavoritesMenu",
		"GiftMenu",
		"InventoryMenu",
		"Journal Menu",
		"Kinect Menu",
		"LoadingMenu",
		"Lockpicking Menu",
		"MagicMenu",
		"PipboyMenu",
		"LevelUpMenu",
		"MainMenu",
		"PauseMenu",
		"MapMarkerText3D",
		"MapMenu",
		"MessageBoxMenu",
		"Mist Menu",
		"Quantity Menu",
		"RaceSex Menu",
		"Sleep/Wait Menu",
		"StatsMenuSkillRing",
		"StatsMenuPerks",
		"Training Menu",
		"Tutorial Menu",
		"TweenMenu"
	};

	std::unordered_map<std::string, bool> menuTypes({
		{ "BarterMenu", false },
		{ "Book Menu", false },
		{ "Console", false },
		{ "Native UI Menu", false },
		{ "ContainerMenu", false },
		{ "Dialogue Menu", false },
		{ "Crafting Menu", false },
		{ "Credits Menu", false },
		{ "Cursor Menu", false },
		{ "Debug Text Menu", false },
		{ "FaderMenu", false },
		{ "FavoritesMenu", false },
		{ "GiftMenu", false },
		{ "HUDMenu", false },
		{ "InventoryMenu", false },
		{ "Journal Menu", false },
		{ "Kinect Menu", false },
		{ "LoadingMenu", false },
		{ "LevelUpMenu", false },
		{ "Lockpicking Menu", false },
		{ "MagicMenu", false },
		{ "MainMenu", false },
		{ "MapMarkerText3D", false },
		{ "MapMenu", false },
		{ "MessageBoxMenu", false },
		{ "Mist Menu", false },
		{ "Overlay Interaction Menu", false },
		{ "Overlay Menu", false },
		{ "Quantity Menu", false },
		{ "RaceSex Menu", false },
		{ "Sleep/Wait Menu", false },
		{ "StatsMenu", false },
		{ "StatsMenuPerks", false },
		{ "StatsMenuSkillRing", false },
		{ "TitleSequence Menu", false },
		{ "Top Menu", false },
		{ "Training Menu", false },
		{ "Tutorial Menu", false },
		{ "TweenMenu", false },
		{ "WSEnemyMeters", false },
		{ "WSDebugOverlay", false },
		{ "WSActivateRollover", false },
		{ "WSPrimaryTouchpadInput", false },
		{ "WSSecondaryTouchpadInput", false },
		{ "PipboyMenu", false },
		{ "PauseMenu", false },
		{ "WSCompass", false },
		{ "WSEnemyHealth", false },
		{ "WSLootMenu", false },
		{ "WSHMDHUDInfo", false },
		{ "WSHMDHUDStatus", false },
		{ "WSInteractRolloverPrimary", false },
		{ "WSInteractRolloverSecondary", false },
		{ "WSPrimaryWandHUD", false },
		{ "WSScope", false },
		{ "VATSMenu", false },
		{ "PowerArmorHUDMenu", false },
		{ "WSPowerArmorOverlay", false },
		{ "LoadWaitSpinner", false }
		});

	//Menu open event functions



	//AllMenuEventHandler menuEvent;
	//	
	//EventResult AllMenuEventHandler::ReceiveEvent(MenuOpenCloseEvent* evn, BSTEventDispatcher<MenuOpenCloseEvent> * dispatcher)
	//{
	//	if (!evn)
	//		return kEvent_Continue;

	//	const char * menuName = evn->menuName.c_str();

	//	if (evn->isOpen) //Menu opened
	//	{
	//		LOG("Menu %s opened.", menuName);
	//		if (menuTypes.find(menuName) != menuTypes.end())
	//		{
	//			menuTypes[menuName] = true;
	//		}
	//	}
	//	else  //Menu closed
	//	{
	//		LOG("Menu %s closed.", menuName);
	//		if (menuTypes.find(menuName) != menuTypes.end())
	//		{
	//			menuTypes[menuName] = false;
	//		}
	//	}

	//	return kEvent_Continue;
	//}

	bool isGameStopped()
	{
		for (int i = 0; i < gameStoppingMenus.size(); i++)
		{
			if (menuTypes[gameStoppingMenus[i]] == true)
				return true;
		}
		return false;
	}

	bool isGameStoppedNoDialogue()
	{
		for (int i = 0; i < gameStoppingMenusNoDialogue.size(); i++)
		{
			if (menuTypes[gameStoppingMenusNoDialogue[i]] == true)
				return true;
		}
		return false;
	}

	bool isVatsActive()
	{
		return menuTypes["VATSMenu"] == true;
	}
}

