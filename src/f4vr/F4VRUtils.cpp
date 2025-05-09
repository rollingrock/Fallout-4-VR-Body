#include "F4VRUtils.h"

#include <f4se/BSGeometry.h>
#include <f4se/GameSettings.h>

#include "../Config.h"
#include "f4se/PapyrusEvents.h"

namespace f4vr {
	// TODO: move those to the VR handler that will handle all controls logic like press
	static bool _controlsThumbstickEnableState = true;
	static float _controlsThumbstickOriginalDeadzone = 0.25f;
	static float _controlsThumbstickOriginalDeadzoneMax = 0.94f;
	static float _controlsDirectionalOriginalDeadzone = 0.5f;

	/**
	 * If to enable/disable the use of both controllers analog thumbstick.
	 */
	void setControlsThumbstickEnableState(const bool toEnable) {
		if (_controlsThumbstickEnableState == toEnable) {
			return; // no change
		}
		_controlsThumbstickEnableState = toEnable;
		if (toEnable) {
			setINIFloat("fLThumbDeadzone:Controls", _controlsThumbstickOriginalDeadzone);
			setINIFloat("fLThumbDeadzoneMax:Controls", _controlsThumbstickOriginalDeadzoneMax);
			setINIFloat("fDirectionalDeadzone:Controls", _controlsDirectionalOriginalDeadzone);
		} else {
			_controlsThumbstickOriginalDeadzone = GetINISetting("fLThumbDeadzone:Controls")->data.f32;
			_controlsThumbstickOriginalDeadzoneMax = GetINISetting("fLThumbDeadzoneMax:Controls")->data.f32;
			_controlsDirectionalOriginalDeadzone = GetINISetting("fDirectionalDeadzone:Controls")->data.f32;
			setINIFloat("fLThumbDeadzone:Controls", 1.0);
			setINIFloat("fLThumbDeadzoneMax:Controls", 1.0);
			setINIFloat("fDirectionalDeadzone:Controls", 1.0);
		}
	}

	void setINIBool(const BSFixedString name, bool value) {
		CallGlobalFunctionNoWait2<BSFixedString, bool>("Utility", "SetINIBool", BSFixedString(name.c_str()), value);
	}

	void setINIFloat(const BSFixedString name, float value) {
		CallGlobalFunctionNoWait2<BSFixedString, float>("Utility", "SetINIFloat", BSFixedString(name.c_str()), value);
	}

	/**
	 * Find a node by the given name in the tree under the other given node recursively.
	 */
	NiNode* getNode(const char* name, NiNode* fromNode) {
		if (!fromNode || !fromNode->m_name) {
			return nullptr;
		}

		if (_stricmp(name, fromNode->m_name.c_str()) == 0) {
			return fromNode;
		}

		for (UInt16 i = 0; i < fromNode->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = fromNode->m_children.m_data[i] ? fromNode->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (const auto ret = getNode(name, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	/**
	 * Find a node by the given name in the tree under the other given node recursively.
	 * This one handles not only NiNode but also BSTriShape.
	 */
	NiNode* getNode2(const char* name, NiNode* fromNode) {
		if (!fromNode || !fromNode->m_name) {
			return nullptr;
		}

		if (_stricmp(name, fromNode->m_name.c_str()) == 0) {
			return fromNode;
		}

		if (!fromNode->m_children.m_data) {
			return nullptr;
		}

		// TODO: use better code
		for (UInt16 i = 0; i < fromNode->m_children.m_emptyRunStart && fromNode->m_children.m_emptyRunStart < 5000; ++i) {
			if (const auto nextNode = static_cast<NiNode*>(fromNode->m_children.m_data[i])) {
				if (const auto ret = getNode2(name, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	/**
	 * Change flags to show or hide a node
	 */
	void setVisibility(NiAVObject* nde, const bool show) {
		if (nde) {
			nde->flags = show ? nde->flags & ~0x1 : nde->flags | 0x1;
		}
	}

	void updateDown(NiNode* nde, const bool updateSelf) {
		if (!nde) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (updateSelf) {
			nde->UpdateWorldData(ud);
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i]) {
				if (const auto niNode = nextNode->GetAsNiNode()) {
					updateDown(niNode, true);
				} else if (const auto triNode = nextNode->GetAsBSGeometry()) {
					triNode->UpdateWorldData(ud);
				}
			}
		}
	}

	void updateDownTo(NiNode* toNode, NiNode* fromNode, const bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		if (updateSelf) {
			NiAVObject::NiUpdateData* ud = nullptr;
			fromNode->UpdateWorldData(ud);
		}

		if (_stricmp(toNode->m_name.c_str(), fromNode->m_name.c_str()) == 0) {
			return;
		}

		for (UInt16 i = 0; i < fromNode->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = fromNode->m_children.m_data[i]) {
				if (const auto niNode = nextNode->GetAsNiNode()) {
					updateDownTo(toNode, niNode, true);
				}
			}
		}
	}

	void updateUpTo(NiNode* toNode, NiNode* fromNode, const bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (_stricmp(toNode->m_name.c_str(), fromNode->m_name.c_str()) == 0) {
			if (updateSelf) {
				fromNode->UpdateWorldData(ud);
			}
			return;
		}

		fromNode->UpdateWorldData(ud);
		if (const auto parent = fromNode->m_parent ? fromNode->m_parent->GetAsNiNode() : nullptr) {
			updateUpTo(toNode, parent, true);
		}
	}
}
