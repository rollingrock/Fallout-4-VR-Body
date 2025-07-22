#pragma once

#include "f4sevr/PapyrusNativeFunctions.h"

namespace frik
{
    constexpr auto BONE_SPHERE_EVEN_NAME = "OnBoneSphereEvent";

    enum class BoneSphereEvent : uint8_t
    {
        None = 0,
        Enter = 1,
        Exit = 2,
    };

    class BoneSphere
    {
    public:
        BoneSphere()
        {
            radius = 0;
            bone = nullptr;
            stickyRight = false;
            stickyLeft = false;
            turnOnDebugSpheres = false;
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;
            debugSphere = nullptr;
        }

        BoneSphere(const float a_radius, RE::NiNode* a_bone, const RE::NiPoint3 a_offset) :
            radius(a_radius), bone(a_bone), offset(a_offset)
        {
            stickyRight = false;
            stickyLeft = false;
            turnOnDebugSpheres = false;
            debugSphere = nullptr;
        }

        float radius;
        RE::NiNode* bone;
        RE::NiPoint3 offset;
        bool stickyRight;
        bool stickyLeft;
        bool turnOnDebugSpheres;
        RE::NiNode* debugSphere;
    };

    class BoneSpheresHandler
    {
    public:
        virtual ~BoneSpheresHandler() { _instance = nullptr; }

        void init();
        void onFrameUpdate();

        std::uint32_t registerBoneSphere(float radius, const F4SEVR::BSFixedString& bone);
        std::uint32_t registerBoneSphereOffset(float radius, const F4SEVR::BSFixedString& bone, F4SEVR::VMArray<float> pos);
        void destroyBoneSphere(std::uint32_t handle);
        void registerForBoneSphereEvents(F4SEVR::VMObject* scriptObj);
        void unRegisterForBoneSphereEvents(F4SEVR::VMObject* scriptObj);
        void toggleDebugBoneSpheres(bool turnOn) const;
        void toggleDebugBoneSpheresAtBone(std::uint32_t handle, bool turnOn);

    private:
        static std::uint32_t registerBoneSphereFunc(F4SEVR::StaticFunctionTag* base, float radius, F4SEVR::BSFixedString bone);
        static std::uint32_t registerBoneSphereOffsetFunc(F4SEVR::StaticFunctionTag* base, float radius, F4SEVR::BSFixedString bone, F4SEVR::VMArray<float> pos);
        static void destroyBoneSphereFunc(F4SEVR::StaticFunctionTag* base, std::uint32_t handle);
        static void registerForBoneSphereEventsFunc(F4SEVR::StaticFunctionTag* base, F4SEVR::VMObject* scriptObj);
        static void unRegisterForBoneSphereEventsFunc(F4SEVR::StaticFunctionTag* base, F4SEVR::VMObject* scriptObj);
        static void toggleDebugBoneSpheresFunc(F4SEVR::StaticFunctionTag* base, bool turnOn);
        static void toggleDebugBoneSpheresAtBoneFunc(F4SEVR::StaticFunctionTag* base, std::uint32_t handle, bool turnOn);
        void sendPapyrusEventToRegisteredScripts(BoneSphereEvent event, std::uint32_t handle, std::uint32_t device);

        void detectBoneSphere();
        void handleDebugBoneSpheres();

        //
        std::unordered_map<std::uint64_t, std::string> _boneSphereEventRegs;

        std::map<std::uint32_t, BoneSphere*> _boneSphereRegisteredObjects;
        std::uint32_t _nextBoneSphereHandle = 1;
        std::uint32_t _curDevice = 0;

        // workaround as papyrus registration requires global functions.
        inline static BoneSpheresHandler* _instance = nullptr;
    };
}
