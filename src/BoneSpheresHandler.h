#pragma once

// TODO: commonlibf4 migration (verify this code)
// Forward declarations for event registration
template <typename T = void>
class RegistrationSetHolder
{
public:
    struct Registration
    {
        std::uint64_t handle;
        RE::BSFixedString scriptName;

        Registration(std::uint64_t a_handle, const RE::BSFixedString& a_scriptName) :
            handle(a_handle), scriptName(a_scriptName) {}
    };

    std::vector<Registration> data;

    void Register(std::uint64_t handle, const RE::BSFixedString& scriptName)
    {
        data.emplace_back(handle, scriptName);
    }

    void Unregister(std::uint64_t handle, const RE::BSFixedString& scriptName)
    {
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it->handle == handle && it->scriptName == scriptName) {
                data.erase(it);
                break;
            }
        }
    }

    template <typename Func>
    void ForEach(Func&& func)
    {
        for (const auto& reg : data) {
            func(reg);
        }
    }
};

// TODO: commonlibf4 migration (verify this code)
// Helper function to send Papyrus events with 3 parameters
template <typename T1, typename T2, typename T3>
void SendPapyrusEvent3(std::uint64_t handle, const RE::BSFixedString& scriptName, const char* eventName, T1 param1, T2 param2, T3 param3)
{
    if (auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton()) {
        // Create event arguments
        auto argsFunc = [=](RE::BSScrapArray<RE::BSScript::Variable>& args) -> bool {
            args.resize(3);
            args[0] = RE::BSScript::Variable();
            args[0] = param1;
            args[1] = RE::BSScript::Variable();
            args[1] = param2;
            args[2] = RE::BSScript::Variable();
            args[2] = param3;
            return true;
        };

        // No filter - send to all objects
        auto filterFunc = [](const RE::BSTSmartPointer<RE::BSScript::Object>&) -> bool {
            return true;
        };

        vm->SendEvent(handle, eventName, argsFunc, filterFunc, nullptr);
    }
}

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

        std::uint32_t registerBoneSphere(float radius, RE::BSFixedString bone);
        std::uint32_t registerBoneSphereOffset(float radius, RE::BSFixedString bone, std::vector<float> pos);
        void destroyBoneSphere(std::uint32_t handle);
        void registerForBoneSphereEvents(RE::BSScript::Object* scriptObj);
        void unRegisterForBoneSphereEvents(RE::BSScript::Object* scriptObj);
        void toggleDebugBoneSpheres(bool turnOn) const;
        void toggleDebugBoneSpheresAtBone(std::uint32_t handle, bool turnOn);

    private:
        static bool registerPapyrusFunctionsCallback(RE::BSScript::IVirtualMachine* vm);
        static std::uint32_t registerBoneSphereFunc(RE::BSScript::Object*, float radius, RE::BSFixedString bone);
        static std::uint32_t registerBoneSphereOffsetFunc(RE::BSScript::Object*, float radius, RE::BSFixedString bone, std::vector<float> pos);
        static void destroyBoneSphereFunc(RE::BSScript::Object*, std::uint32_t handle);
        static void registerForBoneSphereEventsFunc(RE::BSScript::Object*, RE::BSScript::Object* scriptObj);
        static void unRegisterForBoneSphereEventsFunc(RE::BSScript::Object*, RE::BSScript::Object* scriptObj);
        static void toggleDebugBoneSpheresFunc(RE::BSScript::Object*, bool turnOn);
        static void toggleDebugBoneSpheresAtBoneFunc(RE::BSScript::Object*, std::uint32_t handle, bool turnOn);

        void detectBoneSphere();
        void handleDebugBoneSpheres();

        RegistrationSetHolder<> _boneSphereEventRegs;

        std::map<std::uint32_t, BoneSphere*> _boneSphereRegisteredObjects;
        std::uint32_t _nextBoneSphereHandle = 1;
        std::uint32_t _curDevice = 0;

        // workaround as papyrus registration requires global functions.
        inline static BoneSpheresHandler* _instance = nullptr;
    };
}
