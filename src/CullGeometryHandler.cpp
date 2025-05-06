#include "CullGeometryHandler.h"
#include "Config.h"
#include "Debug.h"
#include "F4VRBody.h"
#include "utils.h"

namespace frik {
	/// <summary>
	/// Hide/Show player specific equipment slot found by index.
	/// </summary>
	static void setEquipmentSlotByIndexVisibility(const int slotId, const bool toHide) {
		const auto& slot = (*g_player)->equipData->slots[slotId];

		if (slot.item == nullptr || slot.node == nullptr) {
			return;
		}

		const auto formType = slot.item->GetFormType();
		if (formType != kFormType_ARMO) {
			return;
		}

		showHideNode(slot.node, toHide);
	}

	/// <summary>
	/// Pre-calculate the indexes of the face and skin geometries to hide.
	/// This is performance optimization to avoid iterating over all the geometries every frame by only doing it once per second.
	/// It reduces the "cullPlayerGeometry" time from ~0.05ms to ~0.0002ms (for 89 frames, if framerate is 90).
	/// May not sounds as much but total of this mod update frame is 0.25ms making it 20% of the time!
	/// And god dammit the game is slow enough not to waste more time.
	/// <p>
	/// The geometry array can change when equipment is changed, weapon is drawned, etc. To handle it we check if the last
	/// hidden geometry didn't change. If it did, we re-calculate the indexes to hide.
	/// </summary>
	void CullGeometryHandler::preProcessHideGeometryIndexes(BSFadeNode* rn) {
		const auto now = std::time(nullptr);
		if (now - _lastPreProcessTime < 2) {
			// check that the geometries array didn't change by verifying the last hidden geometry is the same we expect
			if (_lastHiddenGeometryIdx >= 0 && _lastHiddenGeometryIdx < rn->kGeomArray.count) {
				const auto gemName = std::string(rn->kGeomArray[_lastHiddenGeometryIdx].spGeometry->m_name.c_str());
				if (_lastHiddenGeometryName == gemName) {
					return;
				}
			}
		}
		_lastPreProcessTime = now;

		_hideFaceSkinGeometryIndexes.clear();
		for (auto i = 0; i < rn->kGeomArray.count; i++) {
			auto& geometry = rn->kGeomArray[i].spGeometry;
			const auto geomName = std::string(geometry->m_name.c_str());
			auto geomStr = str_tolower(trim(geomName));

			bool toHide = false;
			if (g_config->hideHead) {
				for (auto& faceGeom : g_config->faceGeometry) {
					if (geomStr.find(faceGeom) != std::string::npos) {
						toHide = true;
						break;
					}
				}
			}

			if (g_config->hideSkin && !toHide) {
				for (auto& skinGeom : g_config->skinGeometry) {
					if (geomStr.find(skinGeom) != std::string::npos) {
						toHide = true;
						break;
					}
				}
			}

			// in case it was hidden before and shouldn't be anymore
			showHideNode(geometry, false);

			if (toHide) {
				_hideFaceSkinGeometryIndexes.push_back(i);
				_lastHiddenGeometryIdx = i;
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
		if (c_selfieMode && g_config->selfieIgnoreHideFlags) {
			restoreGeometry();
			return;
		}

		const auto rn = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode);
		if (!rn) {
			return;
		}

		// check for selfie mode to handle an edge-case where all hide setting are set to false but the geometries are not restored
		// preProcessHideGeometryIndexes will restore them (even equipment) so it's a hacky fix
		if (g_config->hideHead || g_config->hideSkin || c_selfieMode) {
			preProcessHideGeometryIndexes(rn);
			for each (int idx in _hideFaceSkinGeometryIndexes) {
				showHideNode(rn->kGeomArray[idx].spGeometry, true);
			}
		}

		if (g_config->hideEquipment) {
			for (const auto slot : g_config->hideEquipSlotIndexes) {
				setEquipmentSlotByIndexVisibility(slot, true);
			}
		}

		if (g_config->checkDebugDumpDataOnceFor("geometry")) {
			dumpPlayerGeometry(rn);
		}
	}

	/// <summary>
	/// Show all player geometries and equipment slots.
	/// </summary>
	void CullGeometryHandler::restoreGeometry() {
		//Face and Skin
		const auto rn = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode);
		if (rn) {
			for (auto i = 0; i < rn->kGeomArray.count; ++i) {
				showHideNode(rn->kGeomArray[i].spGeometry, false);
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
}
