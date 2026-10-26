#pragma once

#include <string>
#include <vector>

#include "f4sevr/PapyrusNativeFunctions.h"

namespace frik
{
    constexpr auto BONE_SPHERE_EVEN_NAME = "OnBoneSphereEvent";
    constexpr auto DEBUG_SPHERE_NODE_NAME = "Sphere01";

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

        BoneSphere(const float a_radius, RE::NiNode* a_bone, const RE::NiPoint3 a_offset)
            : radius(a_radius),
              bone(a_bone),
              boneName(a_bone ? a_bone->name.c_str() : ""),
              offset(a_offset)
        {
            stickyRight = false;
            stickyLeft = false;
            turnOnDebugSpheres = false;
            debugSphere = nullptr;
        }

        float radius;
        // the live bone, re-resolved by name after a skeleton rebuild (null until it is)
        RE::NiNode* bone;
        std::string boneName;
        RE::NiPoint3 offset;
        bool stickyRight;
        bool stickyLeft;
        bool turnOnDebugSpheres;
        RE::NiNode* debugSphere;
        // found in the player's own 3D at registration: re-resolved only there, never in the whole scene, so a rebuild
        // cannot move the sphere onto another actor's or an empty power armor frame's bone of the same name
        bool inPlayerTree = true;
        // the skeleton generation a by-name lookup was last attempted in, and the frames of it left to retry in, so a
        // bone missing on a rebuild's first frame is still found while one that is not in this skeleton stops costing walks
        std::optional<std::uint32_t> resolveGeneration;
        std::uint32_t resolveFramesLeft = 0;
    };

    class BoneSpheresHandler
    {
    public:
        virtual ~BoneSpheresHandler()
        {
            _instance = nullptr;
        }

        void init();
        void onFrameUpdate();
        // the skeleton nodes the spheres hang on are going away; forget them and re-resolve on the next frame that has them
        void onSkeletonReleased();

        std::uint32_t registerBoneSphere(float radius, const F4SEVR::BSFixedString& bone);
        std::uint32_t registerBoneSphereOffset(float radius, const F4SEVR::BSFixedString& bone, F4SEVR::VMArray<float> pos);
        void destroyBoneSphere(std::uint32_t handle);
        void registerForBoneSphereEvents(F4SEVR::VMObject* scriptObj);
        void unRegisterForBoneSphereEvents(F4SEVR::VMObject* scriptObj);
        void toggleDebugBoneSpheres(bool turnOn) const;
        void toggleDebugBoneSpheresAtBone(std::uint32_t handle, bool turnOn);

        // the registrations as plain values, for the devbench probe
        struct SphereInfo
        {
            std::uint32_t handle;
            std::string boneName;
            float radius;
            bool hasBone;
            bool debugOn;
            bool hasDebugSphere;
        };

        std::vector<SphereInfo> getSphereInfos() const;

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
        void detachOrphanDebugSpheres(RE::NiNode* bone) const;
        static RE::NiNode* findBoneNode(const char* name, bool inPlayerTree);
        // the sphere's bone, re-resolved by name when a rebuild dropped it; null while the bone does not exist
        static RE::NiNode* liveBone(BoneSphere& sphere);

        //
        std::unordered_map<std::uint64_t, std::string> _boneSphereEventRegs;

        std::map<std::uint32_t, BoneSphere*> _boneSphereRegisteredObjects;
        std::uint32_t _nextBoneSphereHandle = 1;
        std::uint32_t _curDevice = 0;

        // workaround as papyrus registration requires global functions.
        inline static BoneSpheresHandler* _instance = nullptr;
    };
}
