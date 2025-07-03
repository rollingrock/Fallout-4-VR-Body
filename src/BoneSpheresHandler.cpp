#include "BoneSpheresHandler.h"

#include <ranges>

#include "FRIK.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"

using namespace common;

namespace frik
{
    void BoneSpheresHandler::init()
    {
        if (_instance) {
            throw std::exception("Bone Spheres Handler is already initialized, only single instance can be used!");
        }
        _instance = this;
        f4vr::registerPapyrusNativeFunctions(registerPapyrusFunctionsCallback);
    }

    /**
     * Register code for Papyrus scripts.
     */
    bool BoneSpheresHandler::registerPapyrusFunctionsCallback(RE::BSScript::IVirtualMachine* vm)
    {
        // TODO: commonlibf4 migration
        // vm->BindNativeMethod("FRIK:FRIK", "RegisterBoneSphere", registerBoneSphereFunc);
        // vm->BindNativeMethod("FRIK:FRIK", "RegisterBoneSphereOffset", registerBoneSphereOffsetFunc);
        // vm->BindNativeMethod("FRIK:FRIK", "DestroyBoneSphere", destroyBoneSphereFunc);
        // vm->BindNativeMethod("FRIK:FRIK", "RegisterForBoneSphereEvents", registerForBoneSphereEventsFunc);
        // vm->BindNativeMethod("FRIK:FRIK", "UnRegisterForBoneSphereEvents", unRegisterForBoneSphereEventsFunc);
        // vm->BindNativeMethod("FRIK:FRIK", "toggleDebugBoneSpheres", toggleDebugBoneSpheresFunc);
        // vm->BindNativeMethod("FRIK:FRIK", "toggleDebugBoneSpheresAtBone", toggleDebugBoneSpheresAtBoneFunc);
        return true;
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereFunc(RE::BSScript::Object*, const float radius, const RE::BSFixedString bone)
    {
        return _instance->registerBoneSphere(radius, bone);
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereOffsetFunc(RE::BSScript::Object*, const float radius, const RE::BSFixedString bone, std::vector<float> pos)
    {
        return _instance->registerBoneSphereOffset(radius, bone, pos);
    }

    void BoneSpheresHandler::destroyBoneSphereFunc(RE::BSScript::Object*, const std::uint32_t handle)
    {
        _instance->destroyBoneSphere(handle);
    }

    void BoneSpheresHandler::registerForBoneSphereEventsFunc(RE::BSScript::Object*, RE::BSScript::Object* scriptObj)
    {
        _instance->registerForBoneSphereEvents(scriptObj);
    }

    void BoneSpheresHandler::unRegisterForBoneSphereEventsFunc(RE::BSScript::Object*, RE::BSScript::Object* scriptObj)
    {
        _instance->unRegisterForBoneSphereEvents(scriptObj);
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresFunc(RE::BSScript::Object*, const bool turnOn)
    {
        _instance->toggleDebugBoneSpheres(turnOn);
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresAtBoneFunc(RE::BSScript::Object*, const std::uint32_t handle, const bool turnOn)
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

        // TODO: there must be a lower node we can start the search at
        RE::NiNode* boneNode = f4vr::findChildNode(f4vr::getWorldRootNode(), bone.c_str());

        if (!boneNode) {
            logger::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
            return 0;
        }

        const auto sphere = new BoneSphere(radius, boneNode, RE::NiPoint3(0, 0, 0));
        const std::uint32_t handle = _nextBoneSphereHandle++;

        _boneSphereRegisteredObjects[handle] = sphere;

        return handle;
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereOffset(const float radius, const RE::BSFixedString bone, std::vector<float> pos)
    {
        if (radius == 0.0) {
            return 0;
        }

        if (pos.size() != 3) {
            return 0;
        }

        // TODO: there must be a lower node we can start the search at
        const auto rootNode = f4vr::getWorldRootNode();
        if (!rootNode) {
            logger::info("can't register yet as new game");
            return 0;
        }

        auto boneNode = f4vr::findChildNode(rootNode, bone.c_str());

        if (!boneNode) {
            auto n = rootNode;

            while (n->parent) {
                n = n->parent;
            }

            boneNode = f4vr::findChildNode(n, bone.c_str()); // ObjectLODRoot

            if (!boneNode) {
                logger::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
                return 0;
            }
        }

        RE::NiPoint3 offsetVec;
        offsetVec.x = pos[0];
        offsetVec.y = pos[1];
        offsetVec.z = pos[2];

        const auto sphere = new BoneSphere(radius, boneNode, offsetVec);
        const std::uint32_t handle = _nextBoneSphereHandle++;

        _boneSphereRegisteredObjects[handle] = sphere;

        return handle;
    }

    void BoneSpheresHandler::destroyBoneSphere(const std::uint32_t handle)
    {
        if (_boneSphereRegisteredObjects.contains(handle)) {
            if (const auto sphere = _boneSphereRegisteredObjects[handle]->debugSphere) {
                sphere->flags.flags |= 0x1;
                sphere->local.scale = 0;
                sphere->parent->DetachChild(sphere);
            }

            delete _boneSphereRegisteredObjects[handle];
            _boneSphereRegisteredObjects.erase(handle);
        }
    }

    void BoneSpheresHandler::registerForBoneSphereEvents(RE::BSScript::Object* scriptObj)
    {
        if (!scriptObj) {
            logger::warn("Failed to register for BoneSphereEvents, no scriptObj");
            return;
        }

        logger::info("Register for BoneSphereEvents by Script:'{}'", scriptObj->GetTypeInfo()->GetName());
        _boneSphereEventRegs.Register(scriptObj->GetHandle(), scriptObj->GetTypeInfo()->GetName());
    }

    void BoneSpheresHandler::unRegisterForBoneSphereEvents(RE::BSScript::Object* scriptObj)
    {
        if (!scriptObj) {
            logger::warn("Failed to unregister from BoneSphereEvents, no scriptObj");
            return;
        }

        logger::info("Unregister from BoneSphereEvents by Script:'{}'", scriptObj->GetTypeInfo()->GetName());
        _boneSphereEventRegs.Unregister(scriptObj->GetHandle(), scriptObj->GetTypeInfo()->GetName());
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
        const auto fpSkeleton = f4vr::getFirstPersonSkeleton();
        if (fpSkeleton == nullptr) {
            return;
        }

        // prefer to use fingers but these aren't always rendered.    so default to hand if nothing else

        const RE::NiAVObject* rFinger = f4vr::findChildNode(fpSkeleton, "RArm_Finger22");
        const RE::NiAVObject* lFinger = f4vr::findChildNode(fpSkeleton, "LArm_Finger22");

        if (rFinger == nullptr) {
            rFinger = f4vr::findChildNode(fpSkeleton, "RArm_Hand");
        }

        if (lFinger == nullptr) {
            lFinger = f4vr::findChildNode(fpSkeleton, "LArm_Hand");
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

                    std::int32_t evt = static_cast<std::int32_t>(BoneSphereEvent::Enter);
                    std::uint32_t handle = element.first;
                    std::uint32_t device = 1;
                    _curDevice = device;

                    if (!_boneSphereEventRegs.data.empty()) {
                        auto functor = [&evt, &handle, &device](const auto& reg) {
                            SendPapyrusEvent3<std::int32_t, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            } else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
                if (element.second->stickyRight) {
                    element.second->stickyRight = false;

                    std::int32_t evt = static_cast<std::int32_t>(BoneSphereEvent::Exit);
                    std::uint32_t handle = element.first;
                    _curDevice = 0;

                    if (!_boneSphereEventRegs.data.empty()) {
                        std::uint32_t device = 1;
                        auto functor = [&evt, &handle, &device](const auto& reg) {
                            SendPapyrusEvent3<std::int32_t, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            }

            dist = static_cast<double>(vec3Len(lFinger->world.translate - offset));

            if (dist <= static_cast<double>(element.second->radius) - 0.1) {
                if (!element.second->stickyLeft) {
                    element.second->stickyLeft = true;

                    std::int32_t evt = static_cast<std::int32_t>(BoneSphereEvent::Enter);
                    std::uint32_t handle = element.first;
                    std::uint32_t device = 2;
                    _curDevice = device;

                    if (!_boneSphereEventRegs.data.empty()) {
                        auto functor = [&evt, &handle, &device](const auto& reg) {
                            SendPapyrusEvent3<std::int32_t, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
                        };
                        _boneSphereEventRegs.ForEach(functor);
                    }
                }
            } else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
                if (element.second->stickyLeft) {
                    element.second->stickyLeft = false;

                    std::int32_t evt = static_cast<std::int32_t>(BoneSphereEvent::Exit);
                    std::uint32_t handle = element.first;
                    _curDevice = 0;

                    if (!_boneSphereEventRegs.data.empty()) {
                        std::uint32_t device = 2;
                        auto functor = [&evt, &handle, &device](const auto& reg) {
                            SendPapyrusEvent3<std::int32_t, std::uint32_t, std::uint32_t>(reg.handle, reg.scriptName, BONE_SPHERE_EVEN_NAME, evt, handle, device);
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
                    sphere->flags.flags &= 0xfffffffffffffffe;
                    sphere->local.scale = val->radius * 2;
                    val->debugSphere = sphere;
                }
            } else if (sphere && !val->turnOnDebugSpheres) {
                sphere->flags.flags |= 0x1;
                sphere->local.scale = 0;
            } else if (sphere && val->turnOnDebugSpheres) {
                sphere->flags.flags &= 0xfffffffffffffffe;
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
