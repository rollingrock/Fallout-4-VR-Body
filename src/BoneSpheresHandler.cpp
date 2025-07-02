#include "BoneSpheresHandler.h"

#include <ranges>

#include "FRIK.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"

using namespace common;

namespace frik
{
    void BoneSpheresHandler::init(const F4SE::LoadInterface* f4se)
    {
        if (_instance) {
            throw std::exception("Bone Spheres Handler is already initialized, only single instance can be used!");
        }
        _instance = this;
        f4vr::registerPapyrusNativeFunctions(f4se, registerPapyrusFunctionsCallback);
    }

    /**
     * Register code for Papyrus scripts.
     */
    bool BoneSpheresHandler::registerPapyrusFunctionsCallback(RE::BSScript::Internal::VirtualMachine* vm)
    {
        vm->RegisterFunction(new NativeFunction2("RegisterBoneSphere", "FRIK:FRIK", registerBoneSphereFunc, vm));
        vm->RegisterFunction(new NativeFunction3("RegisterBoneSphereOffset", "FRIK:FRIK", registerBoneSphereOffsetFunc, vm));
        vm->RegisterFunction(new NativeFunction1("DestroyBoneSphere", "FRIK:FRIK", destroyBoneSphereFunc, vm));
        vm->RegisterFunction(new NativeFunction1("RegisterForBoneSphereEvents", "FRIK:FRIK", registerForBoneSphereEventsFunc, vm));
        vm->RegisterFunction(new NativeFunction1("UnRegisterForBoneSphereEvents", "FRIK:FRIK", unRegisterForBoneSphereEventsFunc, vm));
        vm->RegisterFunction(new NativeFunction1("toggleDebugBoneSpheres", "FRIK:FRIK", toggleDebugBoneSpheresFunc, vm));
        vm->RegisterFunction(new NativeFunction2("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", toggleDebugBoneSpheresAtBoneFunc, vm));
        return true;
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereFunc(StaticFunctionTag* base, const float radius, const RE::BSFixedString bone)
    {
        return _instance->registerBoneSphere(radius, bone);
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereOffsetFunc(StaticFunctionTag* base, const float radius, const RE::BSFixedString bone, VMArray<float> pos)
    {
        return _instance->registerBoneSphereOffset(radius, bone, pos);
    }

    void BoneSpheresHandler::destroyBoneSphereFunc(StaticFunctionTag* base, const std::uint32_t handle)
    {
        _instance->destroyBoneSphere(handle);
    }

    void BoneSpheresHandler::registerForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj)
    {
        _instance->registerForBoneSphereEvents(scriptObj);
    }

    void BoneSpheresHandler::unRegisterForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj)
    {
        _instance->unRegisterForBoneSphereEvents(scriptObj);
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresFunc(StaticFunctionTag* base, const bool turnOn)
    {
        _instance->toggleDebugBoneSpheres(turnOn);
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresAtBoneFunc(StaticFunctionTag* base, const std::uint32_t handle, const bool turnOn)
    {
        _instance->toggleDebugBoneSpheresAtBone(handle, turnOn);
    }

    void BoneSpheresHandler::onFrameUpdate()
    {
        detectBoneSphere();
        handleDebugBoneSpheres();
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphere(const float radius, const RE::BSFixedString bone)
    {
        if (radius == 0.0) {
            return 0;
        }

        RE::NiNode* boneNode = f4vr::getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode)->GetAsRE::NiNode();

        if (!boneNode) {
            logger::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
            return 0;
        }

        const auto sphere = new BoneSphere(radius, boneNode, RE::NiPoint3(0, 0, 0));
        const std::uint32_t handle = _nextBoneSphereHandle++;

        _boneSphereRegisteredObjects[handle] = sphere;

        return handle;
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereOffset(const float radius, const RE::BSFixedString bone, VMArray<float> pos)
    {
        if (radius == 0.0) {
            return 0;
        }

        if (pos.Length() != 3) {
            return 0;
        }

        if (!(*g_player)->unkF0) {
            logger::info("can't register yet as new game");
            return 0;
        }

        auto boneNode = f4vr::getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode);

        if (!boneNode) {
            auto n = (*g_player)->unkF0->rootNode->GetAsRE::NiNode();

            while (n->parent) {
                n = n->parent->GetAsRE::NiNode();
            }

            boneNode = f4vr::getChildNode(bone.c_str(), n); // ObjectLODRoot

            if (!boneNode) {
                logger::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
                return 0;
            }
        }

        RE::NiPoint3 offsetVec;

        pos.Get(&offsetVec.x, 0);
        pos.Get(&offsetVec.y, 1);
        pos.Get(&offsetVec.z, 2);

        const auto sphere = new BoneSphere(radius, boneNode, offsetVec);
        const std::uint32_t handle = _nextBoneSphereHandle++;

        _boneSphereRegisteredObjects[handle] = sphere;

        return handle;
    }

    void BoneSpheresHandler::destroyBoneSphere(const std::uint32_t handle)
    {
        if (_boneSphereRegisteredObjects.contains(handle)) {
            if (const auto sphere = _boneSphereRegisteredObjects[handle]->debugSphere) {
                sphere->flags |= 0x1;
                sphere->local.scale = 0;
                sphere->parent->RemoveChild(sphere);
            }

            delete _boneSphereRegisteredObjects[handle];
            _boneSphereRegisteredObjects.erase(handle);
        }
    }

    void BoneSpheresHandler::registerForBoneSphereEvents(VMObject* scriptObj)
    {
        if (!scriptObj) {
            logger::warn("Failed to register for BoneSphereEvents, no scriptObj");
            return;
        }

        logger::info("Register for BoneSphereEvents by Script:'{}'", scriptObj->GetObjectType().c_str());
        _boneSphereEventRegs.Register(scriptObj->GetHandle(), scriptObj->GetObjectType());
    }

    void BoneSpheresHandler::unRegisterForBoneSphereEvents(VMObject* scriptObj)
    {
        if (!scriptObj) {
            logger::warn("Failed to unregister from BoneSphereEvents, no scriptObj");
            return;
        }

        logger::info("Register from BoneSphereEvents by Script:'{}'", scriptObj->GetObjectType().c_str());
        _boneSphereEventRegs.Unregister(scriptObj->GetHandle(), scriptObj->GetObjectType());
    }

    void BoneSpheresHandler::toggleDebugBoneSpheres(const bool turnOn) const
    {
        for (const auto& val : _boneSphereRegisteredObjects | std::views::values) {
            val->turnOnDebugSpheres = turnOn;
        }
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresAtBone(const std::uint32_t handle, const bool turnOn)
    {
        if (_boneSphereRegisteredObjects.contains(handle)) {
            _boneSphereRegisteredObjects[handle]->turnOnDebugSpheres = turnOn;
        }
    }

    /**
     * Bone sphere detection
     */
    void BoneSpheresHandler::detectBoneSphere()
    {
        if ((*g_player)->firstPersonSkeleton == nullptr) {
            return;
        }

        // prefer to use fingers but these aren't always rendered.    so default to hand if nothing else

        const RE::NiAVObject* rFinger = f4vr::getChildNode("RArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsRE::NiNode());
        const RE::NiAVObject* lFinger = f4vr::getChildNode("LArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsRE::NiNode());

        if (rFinger == nullptr) {
            rFinger = f4vr::getChildNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsRE::NiNode());
        }

        if (lFinger == nullptr) {
            lFinger = f4vr::getChildNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsRE::NiNode());
        }

        if (lFinger == nullptr || rFinger == nullptr) {
            return;
        }

        for (const auto& element : _boneSphereRegisteredObjects) {
            RE::NiPoint3 offset = element.second->bone->world.rotate * element.second->offset;
            offset = element.second->bone->world.translate + offset;

            double dist = vec3Len(rFinger->world.translate - offset);

            if (dist <= static_cast<double>(element.second->radius) - 0.1) {
                if (!element.second->stickyRight) {
                    element.second->stickyRight = true;

                    SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Enter);
                    std::uint32_t handle = element.first;
                    std::uint32_t device = 1;
                    _curDevice = device;

                    if (!_boneSphereEventRegs.data.empty()) {
                        auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
                            SendPapyrusEvent3<SInt32, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            } else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
                if (element.second->stickyRight) {
                    element.second->stickyRight = false;

                    SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Exit);
                    std::uint32_t handle = element.first;
                    _curDevice = 0;

                    if (!_boneSphereEventRegs.data.empty()) {
                        std::uint32_t device = 1;
                        auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
                            SendPapyrusEvent3<SInt32, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            }

            dist = static_cast<double>(vec3Len(lFinger->world.translate - offset));

            if (dist <= static_cast<double>(element.second->radius) - 0.1) {
                if (!element.second->stickyLeft) {
                    element.second->stickyLeft = true;

                    SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Enter);
                    std::uint32_t handle = element.first;
                    std::uint32_t device = 2;
                    _curDevice = device;

                    if (!_boneSphereEventRegs.data.empty()) {
                        auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
                            SendPapyrusEvent3<SInt32, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            } else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
                if (element.second->stickyLeft) {
                    element.second->stickyLeft = false;

                    SInt32 evt = static_cast<SInt32>(BoneSphereEvent::Exit);
                    std::uint32_t handle = element.first;
                    _curDevice = 0;

                    if (!_boneSphereEventRegs.data.empty()) {
                        std::uint32_t device = 2;
                        auto functor = [&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
                            SendPapyrusEvent3<SInt32, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            }
        }
    }

    void BoneSpheresHandler::handleDebugBoneSpheres()
    {
        for (const auto& val : _boneSphereRegisteredObjects | std::views::values) {
            RE::NiNode* bone = val->bone;
            RE::NiNode* sphere = val->debugSphere;

            if (val->turnOnDebugSpheres && !val->debugSphere) {
                sphere = vrui::getClonedNiNodeForNifFile("Data/Meshes/FRIK/1x1Sphere.nif");
                if (sphere) {
                    sphere->name = RE::BSFixedString("Sphere01");

                    bone->AttachChild(sphere, true);
                    sphere->flags &= 0xfffffffffffffffe;
                    sphere->local.scale = val->radius * 2;
                    val->debugSphere = sphere;
                }
            } else if (sphere && !val->turnOnDebugSpheres) {
                sphere->flags |= 0x1;
                sphere->local.scale = 0;
            } else if (sphere && val->turnOnDebugSpheres) {
                sphere->flags &= 0xfffffffffffffffe;
                sphere->local.scale = val->radius * 2;
            }

            if (sphere) {
                RE::NiPoint3 offset;

                offset = bone->world.rotate * val->offset;
                offset = bone->world.translate + offset;

                // wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
                sphere->local.translate = bone->world.rotate.Transpose() * (offset - bone->world.translate);
            }
        }
    }
}
