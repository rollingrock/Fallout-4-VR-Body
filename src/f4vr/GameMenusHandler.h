#pragma once

#include <algorithm>
#include <unordered_map>

#include "../common/Logger.h"
#include "f4se/GameMenus.h"

// Adopted from Shizof mod with permission, Thanks Shizof!!

namespace f4vr {
	class GameMenusHandler : public BSTEventSink<RE::MenuOpenCloseEvent> {
	public:
		~GameMenusHandler() override {
			(*g_ui)->menuOpenCloseEventSource.RemoveEventSink(this);
		}

		void init() {
			initGameMenuState();
			(*g_ui)->menuOpenCloseEventSource.AddEventSink(this);
		}

		bool isVatsActive() {
			return _gameMenuState["VATSMenu"] == true;
		}

		bool isInScopeMenu() {
			return _gameMenuState["ScopeMenu"] == true;
		}

		bool isPauseMenuOpen() {
			return _gameMenuState["PauseMenu"] == true;
		}

		bool isGameStopped() {
			return std::ranges::any_of(GAME_STOPPING_MENUS, [this](const std::string& menuName) {
				return _gameMenuState[menuName] == true;
			});
		}

		bool isGameStoppedNoDialogue() {
			return std::ranges::any_of(GAME_STOPPING_MENUS_NO_DIALOGUE, [this](const std::string& menuName) {
				return _gameMenuState[menuName] == true;
			});
		}

		void debugDumpAllMenus() {
			common::logger::info("Current game menu state:");
			for (const auto& [menuName, isOpen] : _gameMenuState) {
				common::logger::infoRaw("{}: {}", menuName.c_str(), isOpen ? "Open" : "Closed");
			}
		}

	private:
		void initGameMenuState() {
			_gameMenuState = {};
			for (const auto& menuName : GAME_MENUS) {
				_gameMenuState.insert_or_assign(menuName, false);
			}
		}

		virtual EventResult ReceiveEvent(RE::MenuOpenCloseEvent* evn, void* dispatcher) override {
			if (evn) {
				const auto& menuName = evn->menuName.c_str();
				if (evn->isOpen) {
					common::logger::debug("Game menu '{}' opened", menuName);
					_gameMenuState.insert_or_assign(menuName, true);
				} else {
					common::logger::debug("Game menu '{}' closed", menuName);
					_gameMenuState.insert_or_assign(menuName, false);
				}
			}
			return kEvent_Continue;
		}

		// each menu and whatever it is open (true) or closed (false)
		std::unordered_map<std::string, bool> _gameMenuState;

		inline static std::vector<std::string> GAME_STOPPING_MENUS_NO_DIALOGUE
		{
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

		inline static std::vector<std::string> GAME_STOPPING_MENUS{
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

		inline static std::vector<std::string> GAME_MENUS{
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
			"FaderMenu",
			"FavoritesMenu",
			"GiftMenu",
			"HUDMenu",
			"InventoryMenu",
			"Journal Menu",
			"Kinect Menu",
			"LoadingMenu",
			"LevelUpMenu",
			"Lockpicking Menu",
			"MagicMenu",
			"MainMenu",
			"MapMarkerText3D",
			"MapMenu",
			"MessageBoxMenu",
			"Mist Menu",
			"Overlay Interaction Menu",
			"Overlay Menu",
			"Quantity Menu",
			"RaceSex Menu",
			"Sleep/Wait Menu",
			"StatsMenu",
			"StatsMenuPerks",
			"StatsMenuSkillRing",
			"TitleSequence Menu",
			"Top Menu",
			"Training Menu",
			"Tutorial Menu",
			"TweenMenu",
			"WSEnemyMeters",
			"WSDebugOverlay",
			"WSActivateRollover",
			"WSPrimaryTouchpadInput",
			"WSSecondaryTouchpadInput",
			"PipboyMenu",
			"PauseMenu",
			"WSCompass",
			"WSEnemyHealth",
			"WSLootMenu",
			"WSHMDHUDInfo",
			"WSHMDHUDStatus",
			"WSInteractRolloverPrimary",
			"WSInteractRolloverSecondary",
			"WSPrimaryWandHUD",
			"WSScope",
			"ScopeMenu",
			"VATSMenu",
			"PowerArmorHUDMenu",
			"WSPowerArmorOverlay",
			"LoadWaitSpinner",
		};
	};
}
