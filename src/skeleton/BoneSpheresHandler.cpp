#include "BoneSpheresHandler.h"

#include <ranges>

#include "FRIK.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4sevr/PapyrusNativeFunctions.h"

using namespace common;
using namespace F4SEVR;

namespace frik
{
    void BoneSpheresHandler::init()
    {
        if (_instance) {
            throw std::exception("Bone Spheres Handler is already initialized, only single instance can be used!");
        }
        _instance = this;

        const auto vm = getGameVM()->m_virtualMachine;
        vm->RegisterFunction(new NativeFunction2("RegisterBoneSphere", "FRIK:FRIK", registerBoneSphereFunc, vm));
        vm->RegisterFunction(new NativeFunction3("RegisterBoneSphereOffset", "FRIK:FRIK", registerBoneSphereOffsetFunc, vm));
        vm->RegisterFunction(new NativeFunction1("DestroyBoneSphere", "FRIK:FRIK", destroyBoneSphereFunc, vm));
        vm->RegisterFunction(new NativeFunction1("RegisterForBoneSphereEvents", "FRIK:FRIK", registerForBoneSphereEventsFunc, vm));
        vm->RegisterFunction(new NativeFunction1("UnRegisterForBoneSphereEvents", "FRIK:FRIK", unRegisterForBoneSphereEventsFunc, vm));
        vm->RegisterFunction(new NativeFunction1("toggleDebugBoneSpheres", "FRIK:FRIK", toggleDebugBoneSpheresFunc, vm));
        vm->RegisterFunction(new NativeFunction2("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", toggleDebugBoneSpheresAtBoneFunc, vm));
    }

    // ReSharper disable once CppParameterMayBeConst
    std::uint32_t BoneSpheresHandler::registerBoneSphereFunc(StaticFunctionTag*, const float radius, BSFixedString bone)
    {
        return _instance->registerBoneSphere(radius, bone);
    }

    // ReSharper disable once CppParameterMayBeConst
    // ReSharper disable once CppPassValueParameterByConstReference
    std::uint32_t BoneSpheresHandler::registerBoneSphereOffsetFunc(StaticFunctionTag*, const float radius, BSFixedString bone, VMArray<float> pos)
    {
        return _instance->registerBoneSphereOffset(radius, bone, pos);
    }

    void BoneSpheresHandler::destroyBoneSphereFunc(StaticFunctionTag*, const std::uint32_t handle)
    {
        _instance->destroyBoneSphere(handle);
    }

    void BoneSpheresHandler::registerForBoneSphereEventsFunc(StaticFunctionTag*, VMObject* scriptObj)
    {
        _instance->registerForBoneSphereEvents(scriptObj);
    }

    void BoneSpheresHandler::unRegisterForBoneSphereEventsFunc(StaticFunctionTag*, VMObject* scriptObj)
    {
        _instance->unRegisterForBoneSphereEvents(scriptObj);
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresFunc(StaticFunctionTag*, const bool turnOn)
    {
        _instance->toggleDebugBoneSpheres(turnOn);
    }

    void BoneSpheresHandler::toggleDebugBoneSpheresAtBoneFunc(StaticFunctionTag*, const std::uint32_t handle, const bool turnOn)
    {
        _instance->toggleDebugBoneSpheresAtBone(handle, turnOn);
    }

    void BoneSpheresHandler::onFrameUpdate()
    {
        detectBoneSphere();
        handleDebugBoneSpheres();
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphere(const float radius, const BSFixedString& bone)
    {
        if (radius == 0.0) {
            return 0;
        }

        const auto boneNode = f4vr::findNode(f4vr::getWorldRootNode(), bone.c_str());
        if (!boneNode) {
            logger::info("RegisterBoneSphere: BONE DOES NOT EXIST!!");
            return 0;
        }

        const auto sphere = new BoneSphere(radius, boneNode, RE::NiPoint3(0, 0, 0));
        const std::uint32_t handle = _nextBoneSphereHandle++;

        _boneSphereRegisteredObjects[handle] = sphere;

        return handle;
    }

    std::uint32_t BoneSpheresHandler::registerBoneSphereOffset(const float radius, const BSFixedString& bone, VMArray<float> pos)
    {
        if (radius == 0.0) {
            return 0;
        }

        if (pos.Length() != 3) {
            return 0;
        }

        if (!f4vr::getPlayer()->unkF0) {
            logger::info("can't register yet as new game");
            return 0;
        }

        auto n = f4vr::getWorldRootNode();
        auto boneNode = f4vr::findNode(n, bone.c_str());

        if (!boneNode) {
            while (n->parent) {
                n = n->parent;
            }

            boneNode = f4vr::findNode(n, bone.c_str()); // ObjectLODRoot

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
                sphere->flags.flags |= 0x1;
                sphere->local.scale = 0;
                sphere->parent->DetachChild(sphere);
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

        const auto scriptName = scriptObj->GetObjectType();
        const auto scriptHandle = scriptObj->GetHandle();
        logger::debug("Register for BoneSphereEvents by Script:'{}' ({})", scriptName.c_str(), scriptHandle);
        _boneSphereEventRegs.insert_or_assign(scriptHandle, scriptName);
    }

    void BoneSpheresHandler::unRegisterForBoneSphereEvents(VMObject* scriptObj)
    {
        if (!scriptObj) {
            logger::warn("Failed to unregister from BoneSphereEvents, no scriptObj");
            return;
        }

        const auto scriptName = scriptObj->GetObjectType();
        const auto scriptHandle = scriptObj->GetHandle();
        logger::debug("UnRegister from BoneSphereEvents by Script:'{}' ({})", scriptName.c_str(), scriptHandle);
        _boneSphereEventRegs.erase(scriptHandle);
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

        const RE::NiAVObject* rFinger = f4vr::findNode(fpSkeleton, "RArm_Finger22");
        const RE::NiAVObject* lFinger = f4vr::findNode(fpSkeleton, "LArm_Finger22");

        if (rFinger == nullptr) {
            rFinger = f4vr::findNode(fpSkeleton, "RArm_Hand");
        }

        if (lFinger == nullptr) {
            lFinger = f4vr::findNode(fpSkeleton, "LArm_Hand");
        }

        if (lFinger == nullptr || rFinger == nullptr) {
            return;
        }

        for (const auto& element : _boneSphereRegisteredObjects) {
            RE::NiPoint3 offset = element.second->bone->world.rotate.Transpose() * (element.second->offset);
            offset = element.second->bone->world.translate + offset;

            double dist = vec3Len(rFinger->world.translate - offset);

            if (dist <= static_cast<double>(element.second->radius) - 0.1) {
                if (!element.second->stickyRight) {
                    element.second->stickyRight = true;

                    const std::uint32_t handle = element.first;
                    constexpr std::uint32_t device = 1;
                    _curDevice = 1;
                    sendPapyrusEventToRegisteredScripts(BoneSphereEvent::Enter, handle, device);
                }
            } else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
                if (element.second->stickyRight) {
                    element.second->stickyRight = false;

                    const std::uint32_t handle = element.first;
                    _curDevice = 0;
                    sendPapyrusEventToRegisteredScripts(BoneSphereEvent::Exit, handle, 1);
                }
            }

            dist = static_cast<double>(vec3Len(lFinger->world.translate - offset));

            if (dist <= static_cast<double>(element.second->radius) - 0.1) {
                if (!element.second->stickyLeft) {
                    element.second->stickyLeft = true;

                    const std::uint32_t handle = element.first;
                    constexpr std::uint32_t device = 2;
                    _curDevice = device;
                    sendPapyrusEventToRegisteredScripts(BoneSphereEvent::Enter, handle, device);
                }
            } else if (dist >= static_cast<double>(element.second->radius) + 0.1) {
                if (element.second->stickyLeft) {
                    element.second->stickyLeft = false;

                    const std::uint32_t handle = element.first;
                    _curDevice = 0;
                    sendPapyrusEventToRegisteredScripts(BoneSphereEvent::Exit, handle, 2);
                }
            }
        }
    }

    /**
     * Send the given bone sphere event to all registered scripts.
     */
    void BoneSpheresHandler::sendPapyrusEventToRegisteredScripts(const BoneSphereEvent event, const std::uint32_t handle, const std::uint32_t device)
    {
        const int evt = static_cast<int>(event);
        for (auto [scriptHandle, scriptName] : _boneSphereEventRegs) {
            logger::debug("Send BoneSphere Event({}) to '{}' ({}): Enter, Handle: {}, Device: {}", evt, scriptName, scriptHandle, handle, device);
            auto arguments = getArgs(evt, handle, device);
            execPapyrusFunction(scriptHandle, scriptName, BONE_SPHERE_EVEN_NAME, arguments);
        }
    }

    void BoneSpheresHandler::handleDebugBoneSpheres()
    {
        for (const auto& val : _boneSphereRegisteredObjects | std::views::values) {
            RE::NiNode* bone = val->bone;
            RE::NiNode* sphere = val->debugSphere;

            if (val->turnOnDebugSpheres && !val->debugSphere) {
                sphere = f4vr::getClonedNiNodeForNifFileSetName("Data/Meshes/FRIK/1x1Sphere.nif");
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

                offset = bone->world.rotate.Transpose() * (val->offset);
                offset = bone->world.translate + offset;

                // wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
                sphere->local.translate = bone->world.rotate * ((offset - bone->world.translate));
            }
        }
    }
}
