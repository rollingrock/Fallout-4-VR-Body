#include "BoneSpheresHandler.h"

#include <ranges>

#include "FRIK.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"

using namespace common;

namespace frik {
	void BoneSpheresHandler::init(const F4SEInterface* f4se) {
		if (_instance) {
			throw std::exception("Bone Spheres Handler is already initialized, only single instance can be used!");
		}
		_instance = this;
		f4vr::registerPapyrusNativeFunctions(f4se, registerPapyrusFunctionsCallback);
	}

	/**
	 * Register code for Papyrus scripts.
	 */
	bool BoneSpheresHandler::registerPapyrusFunctionsCallback(VirtualMachine* vm) {
		vm->RegisterFunction(new NativeFunction2("RegisterBoneSphere", "FRIK:FRIK", registerBoneSphereFunc, vm));
		vm->RegisterFunction(new NativeFunction3("RegisterBoneSphereOffset", "FRIK:FRIK", registerBoneSphereOffsetFunc, vm));
		vm->RegisterFunction(new NativeFunction1("DestroyBoneSphere", "FRIK:FRIK", destroyBoneSphereFunc, vm));
		vm->RegisterFunction(new NativeFunction1("RegisterForBoneSphereEvents", "FRIK:FRIK", registerForBoneSphereEventsFunc, vm));
		vm->RegisterFunction(new NativeFunction1("UnRegisterForBoneSphereEvents", "FRIK:FRIK", unRegisterForBoneSphereEventsFunc, vm));
		vm->RegisterFunction(new NativeFunction1("toggleDebugBoneSpheres", "FRIK:FRIK", toggleDebugBoneSpheresFunc, vm));
		vm->RegisterFunction(new NativeFunction2("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", toggleDebugBoneSpheresAtBoneFunc, vm));
		return true;
	}

	UInt32 BoneSpheresHandler::registerBoneSphereFunc(StaticFunctionTag* base, const float radius, const BSFixedString bone) {
		return _instance->registerBoneSphere(radius, bone);
	}

	UInt32 BoneSpheresHandler::registerBoneSphereOffsetFunc(StaticFunctionTag* base, const float radius, const BSFixedString bone, VMArray<float> pos) {
		return _instance->registerBoneSphereOffset(radius, bone, pos);
	}

	void BoneSpheresHandler::destroyBoneSphereFunc(StaticFunctionTag* base, const UInt32 handle) {
		_instance->destroyBoneSphere(handle);
	}

	void BoneSpheresHandler::registerForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj) {
		_instance->registerForBoneSphereEvents(scriptObj);
	}

	void BoneSpheresHandler::unRegisterForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj) {
		_instance->unRegisterForBoneSphereEvents(scriptObj);
	}

	void BoneSpheresHandler::toggleDebugBoneSpheresFunc(StaticFunctionTag* base, const bool turnOn) {
		_instance->toggleDebugBoneSpheres(turnOn);
	}

	void BoneSpheresHandler::toggleDebugBoneSpheresAtBoneFunc(StaticFunctionTag* base, const UInt32 handle, const bool turnOn) {
		_instance->toggleDebugBoneSpheresAtBone(handle, turnOn);
	}

	void BoneSpheresHandler::onFrameUpdate() {
		detectBoneSphere();
		handleDebugBoneSpheres();
	}

	UInt32 BoneSpheresHandler::registerBoneSphere(const float radius, const BSFixedString bone) {
		if (radius == 0.0) {
			return 0;
		}

		NiNode* boneNode = f4vr::getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode)->GetAsNiNode();

		if (!boneNode) {
			Log::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
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
			Log::info("can't register yet as new game");
			return 0;
		}

		auto boneNode = f4vr::getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode);

		if (!boneNode) {
			auto n = (*g_player)->unkF0->rootNode->GetAsNiNode();

			while (n->m_parent) {
				n = n->m_parent->GetAsNiNode();
			}

			boneNode = f4vr::getChildNode(bone.c_str(), n); // ObjectLODRoot

			if (!boneNode) {
				Log::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
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

	void BoneSpheresHandler::registerForBoneSphereEvents(VMObject* scriptObj) {
		if (!scriptObj) {
			Log::warn("Failed to register for BoneSphereEvents, no scriptObj");
			return;
		}

		Log::info("Register for BoneSphereEvents by Script:'%s'", scriptObj->GetObjectType().c_str());
		_boneSphereEventRegs.Register(scriptObj->GetHandle(), scriptObj->GetObjectType());
	}

	void BoneSpheresHandler::unRegisterForBoneSphereEvents(VMObject* scriptObj) {
		if (!scriptObj) {
			Log::warn("Failed to unregister from BoneSphereEvents, no scriptObj");
			return;
		}

		Log::info("Register from BoneSphereEvents by Script:'%s'", scriptObj->GetObjectType().c_str());
		_boneSphereEventRegs.Unregister(scriptObj->GetHandle(), scriptObj->GetObjectType());
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

		const NiAVObject* rFinger = f4vr::getChildNode("RArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		const NiAVObject* lFinger = f4vr::getChildNode("LArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if (rFinger == nullptr) {
			rFinger = f4vr::getChildNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		}

		if (lFinger == nullptr) {
			lFinger = f4vr::getChildNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
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
						auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
							SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
						};
						_boneSphereEventRegs.ForEach(functor);
					}
				}
			} else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
				if (element.second->stickyRight) {
					element.second->stickyRight = false;

					SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Exit);
					UInt32 handle = element.first;
					_curDevice = 0;

					if (!_boneSphereEventRegs.m_data.empty()) {
						UInt32 device = 1;
						auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
							SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
						};
						_boneSphereEventRegs.ForEach(functor);
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

					if (!_boneSphereEventRegs.m_data.empty()) {
						auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
							SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
						};
						_boneSphereEventRegs.ForEach(functor);
					}
				}
			} else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
				if (element.second->stickyLeft) {
					element.second->stickyLeft = false;

					SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Exit);
					UInt32 handle = element.first;
					_curDevice = 0;

					if (!_boneSphereEventRegs.m_data.empty()) {
						UInt32 device = 2;
						auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
							SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
						};
						_boneSphereEventRegs.ForEach(functor);
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
				sphere = vrui::getClonedNiNodeForNifFile("Data/Meshes/FRIK/1x1Sphere.nif");
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
