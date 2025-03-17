#include "WeaponPositionHandler.h"
#include "Config.h"
#include "Skeleton.h"
#include "F4VRBody.h"

namespace F4VRBody {

	void WeaponPositionHandler::onFrameUpdate() {
		handleWeapon();
		offHandToBarrel();
		offHandToScope();
	}

	void WeaponPositionHandler::handleWeapon() {
		if (!(*g_player)->actorState.IsWeaponDrawn()) {
			return;
		}

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_skelly->getRoot();

		NiNode* weap = _skelly->getNode("Weapon", (*g_player)->firstPersonSkeleton);

		std::string weapname("");
		if ((*g_player)->middleProcess->unk08->equipData) {
			weapname = (*g_player)->middleProcess->unk08->equipData->item->GetFullName();
		}

		if (weap) {

			Matrix44 ident = Matrix44();

			ident.data[2][0] = 1.0;
			ident.data[0][1] = 1.0;
			ident.data[1][2] = 1.0;
			ident.data[3][3] = 1.0;
			_skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_localTransform.rot = ident.make43();
			updateTransforms(dynamic_cast<NiNode*>(_skelly->getPlayerNodes()->primaryWeaponScopeCamera));

			auto newWeapon = _skelly->inPowerArmor() ? weapname + "-PA" : weapname;
			if (newWeapon != _lastWeapon) {
				_lastWeapon = newWeapon;
				_useCustomWeaponOffset = false;
				_useCustomOffHandOffset = false;
				auto lookup = g_config->getWeaponOffsets(weapname, _skelly->inPowerArmor() ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
				if (lookup.has_value()) {
					_useCustomOffHandOffset = true;
					_offhandOffset = lookup.value();
					_MESSAGE("Found offHandOffset for %s pos (%f, %f, %f) scale %f: powerArmor: %d",
						weapname.c_str(), _offhandOffset.pos.x, _offhandOffset.pos.y, _offhandOffset.pos.z, _offhandOffset.scale, _skelly->inPowerArmor());
				}
				lookup = g_config->getWeaponOffsets(weapname, _skelly->inPowerArmor() ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
				if (lookup.has_value()) {
					_useCustomWeaponOffset = true;
					_customTransform = lookup.value();
					_MESSAGE("Found weaponOffset for \"%s\", pos:(%f, %f, %f) scale: %f, inPowerArmor: %d",
						weapname.c_str(), _customTransform.pos.x, _customTransform.pos.y, _customTransform.pos.z, _customTransform.scale, _skelly->inPowerArmor());
				}
				else { // offsets should already be applied if not already saved
					NiPoint3 offset = NiPoint3(-0.94, 0, 0); // apply static VR offset
					NiNode* weapOffset = _skelly->getNode("WeaponOffset", weap);

					if (weapOffset) {
						offset.x -= weapOffset->m_localTransform.pos.y;
						offset.y -= -2.099;
						_MESSAGE("%s: WeaponOffset pos (%f, %f, %f) scale %f",
							weapname.c_str(), weapOffset->m_localTransform.pos.x, weapOffset->m_localTransform.pos.y, weapOffset->m_localTransform.pos.z, weapOffset->m_localTransform.scale);
					}
					weap->m_localTransform.pos += offset;
				}
			}

			if (g_config->leftHandedMode) {
				weap->m_localTransform.pos.x += 6.25f;
				weap->m_localTransform.pos.y += 2.5f;
				weap->m_localTransform.pos.z += 2.75f;
			}
			if (_useCustomWeaponOffset) { // load custom transform
				weap->m_localTransform = _customTransform;
			}
			else // save transform to manipulate
				_customTransform = weap->m_localTransform;
			_skelly->updateDown(weap, true);

			// handle offhand gripping

			static NiPoint3 _offhandFingerBonePos = NiPoint3(0, 0, 0);
			static NiPoint3 bodyPos = NiPoint3(0, 0, 0);
			static float avgHandV[3] = { 0.0f, 0.0f, 0.0f };
			static int fc = 0;
			float handV = 0.0f;

			auto offHandBone = g_config->leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
			auto onHandBone = !g_config->leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
			if (_offHandGripping && g_config->enableOffHandGripping) {

				float handFrameMovement;

				handFrameMovement = vec3_len(rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - _offhandFingerBonePos);

				float bodyFrameMovement = vec3_len(_skelly->getCurrentBodyPos() - bodyPos);
				avgHandV[fc] = abs(handFrameMovement - bodyFrameMovement);

				fc = fc == 2 ? 0 : fc + 1;

				float sum = 0;

				for (int i = 0; i < 3; i++) {
					sum += avgHandV[i];
				}

				handV = sum / 3;

				uint64_t reg = g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;
				if (g_config->onePressGripButton && _hasLetGoGripButton) {
					_offHandGripping = false;
				}
				else if (g_config->enableGripButtonToLetGo && _hasLetGoGripButton) {
					if (reg & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->gripButtonID)) {
						_offHandGripping = false;
						_hasLetGoGripButton = false;
					}
				}
				else if ((handV > g_config->gripLetGoThreshold) && !c_isLookingThroughScope) {
					_offHandGripping = false;
				}
				uint64_t _pressLength = 0;
				if (!_repositionButtonHolding) {
					if (reg & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->repositionButtonID)) {
						_repositionButtonHolding = true;
						_hasLetGoRepositionButton = false;
						_repositionButtonHoldStart = nowMillis();
						_startFingerBonePos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - _skelly->getCurrentBodyPos();
						_offsetPreview = weap->m_localTransform.pos;
						_MESSAGE("Reposition Button Hold start: weapon %s mode: %d", weapname.c_str(), _repositionMode);
					}
				}
				else {
					if (!_repositionModeSwitched && reg & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->offHandActivateButtonID)) {
						_repositionMode = static_cast<RepositionMode>((_repositionMode + 1) % (RepositionMode::total + 1));
						if (_vrhook)
							_vrhook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1 * (_repositionMode + 1), 0.3);
						_repositionModeSwitched = true;
						_MESSAGE("Reposition Mode Switch: weapon %s %d ms mode: %d", weapname.c_str(), _pressLength, _repositionMode);
					}
					else if (_repositionModeSwitched && !(reg & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->offHandActivateButtonID))) {
						_repositionModeSwitched = false;
					}
					_pressLength = nowMillis() - _repositionButtonHoldStart;
					if (!_inRepositionMode && reg & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->repositionButtonID) && _pressLength > g_config->holdDelay) {
						if (_vrhook && _inWeaponRepositionMode)
							_vrhook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1 * (_repositionMode + 1), 0.3);
						_inRepositionMode = _inWeaponRepositionMode;
					}
					else if (!(reg & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->repositionButtonID))) {
						_repositionButtonHolding = false;
						_hasLetGoRepositionButton = true;
						_inRepositionMode = false;
						_endFingerBonePos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - _skelly->getCurrentBodyPos();
						_MESSAGE("Reposition Button Hold stop: weapon %s %d ms mode: %d", weapname.c_str(), _pressLength, _repositionMode);
					}
				}

				if (_offHandGripping) {

					NiPoint3 oH2Bar; // off hand to barrel

					oH2Bar = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - weap->m_worldTransform.pos;

					if (_useCustomOffHandOffset) {
						// TODO: figure out y offset (left-right)
						//NiPoint3 zZeroedOffhandOffset = _offhandOffset.pos;
						//zZeroedOffhandOffset.z = 0;
						//auto fcos = vec3_dot(vec3_norm(zZeroedOffhandOffset), vec3_norm(_offhandOffset.pos));
						//auto fsin = vec3_det(vec3_norm(zZeroedOffhandOffset), vec3_norm(_offhandOffset.pos), NiPoint3(-1, 0, 0));
						//oH2Bar.y -= _offhandOffset.pos.y;
						oH2Bar.z -= _offhandOffset.pos.z;
					}
					oH2Bar.z += 3.5f;

					NiPoint3 barrelVec = NiPoint3(0, 1, 0);

					NiPoint3 scopeVecLoc = oH2Bar;
					oH2Bar = weap->m_worldTransform.rot.Transpose() * vec3_norm(oH2Bar) / weap->m_worldTransform.scale;
					scopeVecLoc = _skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_worldTransform.rot.Transpose() * vec3_norm(scopeVecLoc) / _skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_worldTransform.scale;

					Matrix44 rot;
					//rot.rotateVectoVec(oH2Bar, barrelVec);

					// use Quaternion to rotate the vector onto the other vector to avoid issues with the poles
					_aimAdjust.vec2vec(vec3_norm(oH2Bar), vec3_norm(barrelVec));
					rot = _aimAdjust.getRot();
					_originalWeaponRot = weap->m_localTransform.rot; // save unrotated vector
					weap->m_localTransform.rot = rot.multiply43Left(weap->m_localTransform.rot);

					// rotate scopeParent so scope widget works
					Quaternion scopeQ;
					barrelVec = NiPoint3(1, 0, 0);
					_aimAdjust.vec2vec(vec3_norm(scopeVecLoc), vec3_norm(barrelVec));
					rot = _aimAdjust.getRot();

					_skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_localTransform.rot = rot.multiply43Left(_skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_localTransform.rot);
					//updateTransforms(dynamic_cast<NiNode*>(_skelly->getPlayerNodes()->primaryWeaponScopeCamera));

					_offhandFingerBonePos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos;
					_offhandPos = _offhandFingerBonePos;
					bodyPos = _skelly->getCurrentBodyPos();
					vr::VRControllerAxis_t axis_state = !(g_config->pipBoyButtonArm > 0) ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0];
					if (_repositionButtonHolding && _inWeaponRepositionMode) {
						// this is for a preview of the move. The preview happens one frame before we detect the release so must be processed separately.
						auto end = _offhandPos - _skelly->getCurrentBodyPos();
						auto change = vec3_len(end) > vec3_len(_startFingerBonePos) ? vec3_len(end - _startFingerBonePos) : -vec3_len(end - _startFingerBonePos);
						switch (_repositionMode) {
						case weapon:
							_skelly->showWands(wandMode::offhandWand);
							_offsetPreview.x = change;
							if (axis_state.x != 0.f || axis_state.y != 0.f) { // axis_state y is up and down, which corresponds to reticle z axis
								_offsetPreview.y += axis_state.x;
								_offsetPreview.z -= axis_state.y;
								_DMESSAGE("Updating weapon to (%f,%f) with analog input: (%f, %f)", _offsetPreview.y, _offsetPreview.z, axis_state.x, axis_state.y);
							}
							if (change != 0.f)
								_DMESSAGE("Previewing translation for %s by %f to %f", weapname, change, weap->m_localTransform.pos.x + _offsetPreview.x);
							weap->m_localTransform.pos.x += _offsetPreview.x; // x is a distance delta
							weap->m_localTransform.pos.y = _offsetPreview.y; // y, z are cumulative
							weap->m_localTransform.pos.z = _offsetPreview.z;
							break;
						case offhand:
							_skelly->showWands(wandMode::mainhandWand);
							weap->m_localTransform.rot = _originalWeaponRot;
							break;
						case resetToDefault:
							_skelly->showWands(wandMode::both);
							break;
						}
					}
					//_hasLetGoRepositionButton is always one frame after _repositionButtonHolding
					if (_hasLetGoRepositionButton && _pressLength > 0 && _pressLength < g_config->holdDelay && _inWeaponRepositionMode) {
						_MESSAGE("Updating grip rotation for %s: powerArmor: %d", weapname.c_str(), _skelly->inPowerArmor());
						_customTransform.rot = weap->m_localTransform.rot;
						_hasLetGoRepositionButton = false;
						_useCustomWeaponOffset = true;
						g_config->saveWeaponOffsets(weapname, _customTransform, _skelly->inPowerArmor() ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
					}
					else if (_hasLetGoRepositionButton && _pressLength > 0 && _pressLength > g_config->holdDelay && _inWeaponRepositionMode) {
						switch (_repositionMode) {
						case weapon:
							_MESSAGE("Saving position translation for %s from (%f, %f, %f) -> (%f, %f, %f): powerArmor: %d", weapname.c_str(), weap->m_localTransform.pos.x, weap->m_localTransform.pos.y, weap->m_localTransform.pos.z, _offsetPreview.x, _offsetPreview.y, _offsetPreview.z, _skelly->inPowerArmor());
							weap->m_localTransform.pos.x += _offsetPreview.x; // x is a distance delta
							weap->m_localTransform.pos.y = _offsetPreview.y; // y, z are cumulative
							weap->m_localTransform.pos.z = _offsetPreview.z;
							_customTransform.pos = weap->m_localTransform.pos;
							_useCustomWeaponOffset = true;
							g_config->saveWeaponOffsets(weapname, _customTransform, _skelly->inPowerArmor() ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
							break;
						case offhand:
							_offhandOffset.rot = weap->m_localTransform.rot;
							_offhandOffset.pos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - rt->transforms[_skelly->getBoneInMap(onHandBone)].world.pos;
							_useCustomOffHandOffset = true;
							g_config->saveWeaponOffsets(weapname, _offhandOffset, _skelly->inPowerArmor() ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
							_MESSAGE("Saving offHandOffset (%f, %f, %f,) for %s: powerArmor: %d", _offhandOffset.pos.x, _offhandOffset.pos.y, _offhandOffset.pos.z, weapname, _skelly->inPowerArmor());
							break;
						case resetToDefault:
							_MESSAGE("Resetting grip to defaults for %s: powerArmor: %d", weapname, _skelly->inPowerArmor());
							_useCustomWeaponOffset = false;
							_useCustomOffHandOffset = false;
							g_config->removeWeaponOffsets(weapname, _skelly->inPowerArmor() ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
							g_config->removeWeaponOffsets(weapname, _skelly->inPowerArmor() ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
							_repositionMode = weapon;
							break;
						}
						_skelly->hideWands();
						_hasLetGoRepositionButton = false;
					}
				}
			}
			else {
				_offhandFingerBonePos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos;
				_offhandPos = _offhandFingerBonePos;
				bodyPos = _skelly->getCurrentBodyPos();

				if (fc != 0) {
					for (int i = 0; i < 3; i++) {
						avgHandV[i] = 0.0f;
					}
				}
				fc = 0;

			}

			_skelly->updateDown(weap, true);
		}
	}

	void WeaponPositionHandler::offHandToBarrel() {
		NiNode* weap = _skelly->getNode("Weapon", (*g_player)->firstPersonSkeleton);
		if (!weap || !(*g_player)->actorState.IsWeaponDrawn()) {
			_offHandGripping = false;
			return;
		}
		
		BSFlattenedBoneTree* rt = reinterpret_cast<BSFlattenedBoneTree*>(_skelly->getRoot());
		NiPoint3 barrelVec(0, 1, 0);
		NiPoint3 oH2Bar = g_config->leftHandedMode ? rt->transforms[_skelly->getBoneInMap("RArm_Finger31")].world.pos - weap->m_worldTransform.pos : rt->transforms[_skelly->getBoneInMap("LArm_Finger31")].world.pos - weap->m_worldTransform.pos;
		float len = vec3_len(oH2Bar);
		oH2Bar = weap->m_worldTransform.rot.Transpose() * vec3_norm(oH2Bar) / weap->m_worldTransform.scale;
		float dotP = vec3_dot(vec3_norm(oH2Bar), barrelVec);
		uint64_t reg = g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;

		if (!(reg & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->gripButtonID)))) {
			_hasLetGoGripButton = true;
		}

		if (dotP > 0.955 && len > 10.0) {
			if (!g_config->enableGripButtonToGrap) {
				_offHandGripping = true;
			}
			else if (!g_pipboy->status() && (reg & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->gripButtonID)))) {
				if (_offHandGripping || !_hasLetGoGripButton) {
					return;
				}
				_offHandGripping = true;
				_hasLetGoGripButton = false;
			}
		}
	}

	/* Handle off-hand scope*/
	void WeaponPositionHandler::offHandToScope() {
		NiNode* weap = _skelly->getNode("Weapon", (*g_player)->firstPersonSkeleton);
		if (!weap || !(*g_player)->actorState.IsWeaponDrawn() || !c_isLookingThroughScope) {
			_zoomModeButtonHeld = false;
			return;
		}

		static BSFixedString reticleNodeName = "ReticleNode";
		NiAVObject* scopeRet = weap->GetObjectByName(&reticleNodeName);
		if (!scopeRet) {
			return;
		}

		const std::string scopeName = scopeRet->m_name;
		auto reticlePos = scopeRet->GetAsNiNode()->m_worldTransform.pos;
		auto offset = vec3_len(reticlePos - _offhandPos);
		uint64_t handInput = g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;
		const auto handNearScope = (offset < g_config->scopeAdjustDistance); // hand is close to scope, enable scope specific commands

		// Zoom toggling
		if (handNearScope && !_inRepositionMode) {
			if (!_zoomModeButtonHeld && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->offHandActivateButtonID)))) {
				_zoomModeButtonHeld = true;
				_MESSAGE("Zoom Toggle started");
			}
			else if (_zoomModeButtonHeld && !(handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->offHandActivateButtonID)))) {
				_zoomModeButtonHeld = false;
				_MESSAGE("Zoom Toggle pressed; sending message to switch zoom state");
				g_messaging->Dispatch(g_pluginHandle, 16, nullptr, 0, "FO4VRBETTERSCOPES");
				if (_vrhook) {
					_vrhook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1, 0.3);
				}
			}
		}

		if (_inWeaponRepositionMode) {
			uint64_t _pressLength = nowMillis() - _repositionButtonHoldStart;

			// Detect scope reposition button being held near scope. Only has to start near scope.
			if (handNearScope && !_repositionButtonHolding && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->repositionButtonID)))) {
				_repositionButtonHolding = true;
				_hasLetGoRepositionButton = false;
				_repositionButtonHoldStart = nowMillis();
				_MESSAGE("Reposition Button Hold start: scope %s", scopeName.c_str());
			}
			else if (_repositionButtonHolding && !(handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->repositionButtonID)))) {
				// Was in scope reposition mode and just released button, time to save
				_repositionButtonHolding = false;
				_hasLetGoRepositionButton = true;
				_inRepositionMode = false;
				NiPoint3 msgData(0.f, 1, 0.f);
				g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
				_MESSAGE("Reposition Button Hold stop: scope %s %d ms", scopeName.c_str(), _pressLength);
			}

			// Repositioning does not require hand near scope
			if (!_inRepositionMode && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->repositionButtonID))) && _pressLength > g_config->holdDelay) {
				// Enter reposition mode
				if (_vrhook && _inWeaponRepositionMode) {
					_vrhook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1, 0.3);
				}
				_inRepositionMode = _inWeaponRepositionMode;
			}
			else if (_inRepositionMode) { // In reposition mode for better scopes
				vr::VRControllerAxis_t axis_state = !(g_config->pipBoyButtonArm > 0) ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0];
				if (!_repositionModeSwitched && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->offHandActivateButtonID)))) {
					if (_vrhook) {
						_vrhook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1, 0.3);
					}
					_repositionModeSwitched = true;
					NiPoint3 msgData(0.f, 0.f, 0.f);
					g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
					_MESSAGE("Reposition Mode Reset: scope %s %d ms", scopeName.c_str(), _pressLength);
				}
				else if (_repositionModeSwitched && !(handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config->offHandActivateButtonID)))) {
					_repositionModeSwitched = false;
				}
				else if (axis_state.x != 0 || axis_state.y != 0) { // Axis_state y is up and down, which corresponds to reticle z axis
					NiPoint3 msgData(axis_state.x, 0.f, axis_state.y);
					g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
					_MESSAGE("Moving scope reticle. input: (%f, %f)", axis_state.x, axis_state.y);
				}
			}
		}
	}
}