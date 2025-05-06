#include "BoneSpheresHandler.h"
#include <ranges>
#include "F4VRBody.h"
#include "common/CommonUtils.h"

using namespace common;

namespace frik {
	void BoneSpheresHandler::onFrameUpdate() {
		detectBoneSphere();
		handleDebugBoneSpheres();
	}

	void BoneSpheresHandler::holsterWeapon() {
		// Sends Papyrus Event to holster weapon when inside Pipboy usage zone
		if (_boneSphereEventRegs.m_data.empty()) {
			return;
		}
		SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Holster);
		_boneSphereEventRegs.ForEach(
			[&evt](const EventRegistration<NullParameters>& reg) {
				SendPapyrusEvent1<SInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt);
			}
		);
	}

	void BoneSpheresHandler::drawWeapon() {
		// Sends Papyrus to draw weapon when outside Pipboy usage zone
		if (_boneSphereEventRegs.m_data.empty()) {
			return;
		}
		SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Draw);
		_boneSphereEventRegs.ForEach(
			[&evt](const EventRegistration<NullParameters>& reg) {
				SendPapyrusEvent1<SInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt);
			}
		);
	}

	UInt32 BoneSpheresHandler::registerBoneSphere(const float radius, const BSFixedString bone) {
		if (radius == 0.0) {
			return 0;
		}

		NiNode* boneNode = getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode)->GetAsNiNode();

		if (!boneNode) {
			_MESSAGE("RegisterBoneSphere: BONE DOES NOT EXIST!!");
			return 0;
		}

		const auto sphere = new BoneSphere(radius, boneNode, NiPoint3(0, 0, 0));
		const UInt32 handle = _nextBoneSphereHandle++;

		_boneSphereRegisteredObjects[handle] = sphere;

		return handle;
	}

	UInt32 BoneSpheresHandler::registerBoneSphereOffset(const float radius, const BSFixedString bone, VMArray<float> pos) {
		if (radius == 0.0) {
			return 0;
		}

		if (pos.Length() != 3) {
			return 0;
		}

		if (!(*g_player)->unkF0) {
			_MESSAGE("can't register yet as new game");
			return 0;
		}

		auto boneNode = getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode);

		if (!boneNode) {
			auto n = (*g_player)->unkF0->rootNode->GetAsNiNode();

			while (n->m_parent) {
				n = n->m_parent->GetAsNiNode();
			}

			boneNode = getChildNode(bone.c_str(), n); // ObjectLODRoot

			if (!boneNode) {
				_MESSAGE("RegisterBoneSphere: BONE DOES NOT EXIST!!");
				return 0;
			}
		}

		NiPoint3 offsetVec;

		pos.Get(&offsetVec.x, 0);
		pos.Get(&offsetVec.y, 1);
		pos.Get(&offsetVec.z, 2);

		const auto sphere = new BoneSphere(radius, boneNode, offsetVec);
		const UInt32 handle = _nextBoneSphereHandle++;

		_boneSphereRegisteredObjects[handle] = sphere;

		return handle;
	}

	void BoneSpheresHandler::destroyBoneSphere(const UInt32 handle) {
		if (_boneSphereRegisteredObjects.contains(handle)) {
			if (const auto sphere = _boneSphereRegisteredObjects[handle]->debugSphere) {
				sphere->flags |= 0x1;
				sphere->m_localTransform.scale = 0;
				sphere->m_parent->RemoveChild(sphere);
			}

			delete _boneSphereRegisteredObjects[handle];
			_boneSphereRegisteredObjects.erase(handle);
		}
	}

	void BoneSpheresHandler::registerForBoneSphereEvents(VMObject* thisObject) {
		_MESSAGE("RegisterForBoneSphereEvents");
		if (!thisObject) {
			return;
		}

		_boneSphereEventRegs.Register(thisObject->GetHandle(), thisObject->GetObjectType());
	}

	void BoneSpheresHandler::unRegisterForBoneSphereEvents(VMObject* thisObject) {
		if (!thisObject) {
			return;
		}

		_MESSAGE("UnRegisterForBoneSphereEvents");
		_boneSphereEventRegs.Unregister(thisObject->GetHandle(), thisObject->GetObjectType());
	}

	void BoneSpheresHandler::toggleDebugBoneSpheres(const bool turnOn) const {
		for (const auto& val : _boneSphereRegisteredObjects | std::views::values) {
			val->turnOnDebugSpheres = turnOn;
		}
	}

	void BoneSpheresHandler::toggleDebugBoneSpheresAtBone(const UInt32 handle, const bool turnOn) {
		if (_boneSphereRegisteredObjects.contains(handle)) {
			_boneSphereRegisteredObjects[handle]->turnOnDebugSpheres = turnOn;
		}
	}

	/**
	 * Bone sphere detection
	 */
	void BoneSpheresHandler::detectBoneSphere() {
		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		// prefer to use fingers but these aren't always rendered.    so default to hand if nothing else

		const NiAVObject* rFinger = getChildNode("RArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		const NiAVObject* lFinger = getChildNode("LArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if (rFinger == nullptr) {
			rFinger = getChildNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		}

		if (lFinger == nullptr) {
			lFinger = getChildNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		}

		if (lFinger == nullptr || rFinger == nullptr) {
			return;
		}

		for (const auto& element : _boneSphereRegisteredObjects) {
			NiPoint3 offset = element.second->bone->m_worldTransform.rot * element.second->offset;
			offset = element.second->bone->m_worldTransform.pos + offset;

			double dist = vec3Len(rFinger->m_worldTransform.pos - offset);

			if (dist <= static_cast<double>(element.second->radius) - 0.1) {
				if (!element.second->stickyRight) {
					element.second->stickyRight = true;

					SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Enter);
					UInt32 handle = element.first;
					UInt32 device = 1;
					_curDevice = device;

					if (!_boneSphereEventRegs.m_data.empty()) {
						_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
							}
						);
					}
				}
			} else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
				if (element.second->stickyRight) {
					element.second->stickyRight = false;

					SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Exit);
					UInt32 handle = element.first;
					UInt32 device = 1;
					_curDevice = 0;

					if (_boneSphereEventRegs.m_data.size() > 0) {
						_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
							}
						);
					}
				}
			}

			dist = static_cast<double>(vec3Len(lFinger->m_worldTransform.pos - offset));

			if (dist <= static_cast<double>(element.second->radius) - 0.1) {
				if (!element.second->stickyLeft) {
					element.second->stickyLeft = true;

					SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Enter);
					UInt32 handle = element.first;
					UInt32 device = 2;
					_curDevice = device;

					if (_boneSphereEventRegs.m_data.size() > 0) {
						_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
							}
						);
					}
				}
			} else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
				if (element.second->stickyLeft) {
					element.second->stickyLeft = false;

					SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Exit);
					UInt32 handle = element.first;
					UInt32 device = 2;
					_curDevice = 0;

					if (!_boneSphereEventRegs.m_data.empty()) {
						_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
							}
						);
					}
				}
			}
		}
	}

	void BoneSpheresHandler::handleDebugBoneSpheres() {
		for (const auto& val : _boneSphereRegisteredObjects | std::views::values) {
			NiNode* bone = val->bone;
			NiNode* sphere = val->debugSphere;

			if (val->turnOnDebugSpheres && !val->debugSphere) {
				const NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/1x1Sphere.nif");
				f4vr::NiCloneProcess proc;
				proc.unk18 = Offsets::cloneAddr1;
				proc.unk48 = Offsets::cloneAddr2;

				sphere = Offsets::cloneNode(retNode, &proc);
				if (sphere) {
					sphere->m_name = BSFixedString("Sphere01");

					bone->AttachChild(sphere, true);
					sphere->flags &= 0xfffffffffffffffe;
					sphere->m_localTransform.scale = val->radius * 2;
					val->debugSphere = sphere;
				}
			} else if (sphere && !val->turnOnDebugSpheres) {
				sphere->flags |= 0x1;
				sphere->m_localTransform.scale = 0;
			} else if (sphere && val->turnOnDebugSpheres) {
				sphere->flags &= 0xfffffffffffffffe;
				sphere->m_localTransform.scale = val->radius * 2;
			}

			if (sphere) {
				NiPoint3 offset;

				offset = bone->m_worldTransform.rot * val->offset;
				offset = bone->m_worldTransform.pos + offset;

				// wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
				sphere->m_localTransform.pos = bone->m_worldTransform.rot.Transpose() * (offset - bone->m_worldTransform.pos);
			}
		}
	}
}
