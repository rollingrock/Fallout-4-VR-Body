#include "CullGeometryHandler.h"
#include "Config.h"
#include "Debug.h"
#include "FRIK.h"
#include "utils.h"
#include "common/CommonUtils.h"

using namespace common;

namespace frik {
	/// <summary>
	/// Pre-calculate the indexes of the face and skin geometries to hide.
	/// This is performance optimization to avoid iterating over all the geometries every frame by only doing it once per second.
	/// It reduces the "cullPlayerGeometry" time from ~0.05ms to ~0.0002ms (for 89 frames, if frame-rate is 90).
	/// May not sounds as much but total of this mod update frame is 0.25ms making it 20% of the time!
	/// And god dammit the game is slow enough not to waste more time.
	/// <p>
	/// The geometry array can change when equipment is changed, weapon is drawn, etc. To handle it we check if the last
	/// hidden geometry didn't change. If it did, we re-calculate the indexes to hide.
	/// </summary>
	void CullGeometryHandler::preProcessHideGeometryIndexes(RE::BSFadeNode* rn) {
		const auto now = std::time(nullptr);
		if (now - _lastPreProcessTime < 2) {
			// check that the geometries array didn't change by verifying the last hidden geometry is the same we expect
			if (_lastHiddenGeometryIdx >= 0 && _lastHiddenGeometryIdx < rn->geomArray.size()) {
				const auto gemName = std::string(rn->geomArray[_lastHiddenGeometryIdx].geometry->name.c_str());
				if (_lastHiddenGeometryName == gemName) {
					return;
				}
			}
		}
		_lastPreProcessTime = now;

		_hideFaceSkinGeometryIndexes.clear();
		for (std::uint32_t i = 0; i < rn->geomArray.size(); i++) {
            auto& geometry = rn->geomArray[i].geometry;
			const auto geomName = std::string(geometry->name.c_str());
			auto geomStr = str_tolower(trim(geomName));

			bool toHide = false;
			if (g_config.hideHead) {
				for (auto& faceGeom : g_config.faceGeometry()) {
					if (geomStr.find(faceGeom) != std::string::npos) {
						toHide = true;
						break;
					}
				}
			}

			if (g_config.hideSkin && !toHide) {
				for (auto& skinGeom : g_config.skinGeometry()) {
					if (geomStr.find(skinGeom) != std::string::npos) {
						toHide = true;
						break;
					}
				}
			}

			// in case it was hidden before and shouldn't be anymore
			f4vr::setNodeVisibility(geometry, true);

			if (toHide) {
				_hideFaceSkinGeometryIndexes.push_back(i);
				_lastHiddenGeometryIdx = static_cast<int>(i);
				_lastHiddenGeometryName = geomName;
			}
		}
	}

	/// <summary>
	/// Hide player face/skins geometries and equipment slots.
	/// Face is things like eyes, mouth, hair, etc.
	/// Equipment slots are things like helmet, glasses, etc.
	/// </summary>
	void CullGeometryHandler::cullPlayerGeometry() {
		if (g_frik.getSelfieMode() && g_config.selfieIgnoreHideFlags) {
			restoreGeometry();
			return;
		}

		const auto rn = reinterpret_cast<RE::BSFadeNode*>((*g_player)->unkF0->rootNode);
		if (!rn) {
			return;
		}

		// check for selfie mode to handle an edge-case where all hide setting are set to false but the geometries are not restored
		// preProcessHideGeometryIndexes will restore them (even equipment) so it's a hacky fix
		if (g_config.hideHead || g_config.hideSkin || g_frik.getSelfieMode()) {
			preProcessHideGeometryIndexes(rn);
			for each (int idx in _hideFaceSkinGeometryIndexes) {
                f4vr::setNodeVisibility(rn->geomArray[idx].geometry, false);
			}
		}

		if (g_config.hideEquipment) {
			for (const auto slot : g_config.hideEquipSlotIndexes()) {
				setEquipmentSlotByIndexVisibility(slot, true);
			}
		}

		if (g_config.checkDebugDumpDataOnceFor("geometry")) {
			dumpPlayerGeometry(rn);
		}
	}

	/// <summary>
	/// Show all player geometries and equipment slots.
	/// </summary>
	void CullGeometryHandler::restoreGeometry() {
		//Face and Skin
		if (const auto rn = reinterpret_cast<RE::BSFadeNode*>((*g_player)->unkF0->rootNode)) {
			for (UINT32 i = 0; i < rn->geomArray.size(); ++i) {
                f4vr::setNodeVisibility(rn->geomArray[i].geometry, true);
			}
		}

		//Equipment
		setEquipmentSlotByIndexVisibility(0, false);
		setEquipmentSlotByIndexVisibility(1, false);
		setEquipmentSlotByIndexVisibility(2, false);
		setEquipmentSlotByIndexVisibility(16, false);
		setEquipmentSlotByIndexVisibility(17, false);
		setEquipmentSlotByIndexVisibility(18, false);
		setEquipmentSlotByIndexVisibility(19, false);
		setEquipmentSlotByIndexVisibility(20, false);
		setEquipmentSlotByIndexVisibility(22, false);
	}

	/// <summary>
	/// Hide/Show player specific equipment slot found by index.
	/// </summary>
	void CullGeometryHandler::setEquipmentSlotByIndexVisibility(const int slotId, const bool toHide) {
		const auto& slot = (*g_player)->equipData->slots[slotId];

		if (slot.item == nullptr || slot.node == nullptr) {
			return;
		}

		const auto formType = slot.item->GetFormType();
		if (formType != kFormType_ARMO) {
			return;
		}

		f4vr::setNodeVisibility(slot.node, !toHide);
	}
}
