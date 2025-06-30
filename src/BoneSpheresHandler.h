#pragma once

#include "f4se/PapyrusEvents.h"

namespace frik {
	constexpr auto BONE_SPHERE_EVEN_NAME = "OnBoneSphereEvent";

	enum class BoneSphereEvent : uint8_t {
		None = 0,
		Enter = 1,
		Exit = 2,
	};

	class BoneSphere {
	public:
		BoneSphere() {
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
			: radius(a_radius), bone(a_bone), offset(a_offset) {
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

	class BoneSpheresHandler {
	public:
		virtual ~BoneSpheresHandler() { _instance = nullptr; }

		void init(const F4SE::detail::F4SEInterface* f4se);
		void onFrameUpdate();

		std::uint32_t registerBoneSphere(float radius, RE::BSFixedString bone);
		std::uint32_t registerBoneSphereOffset(float radius, RE::BSFixedString bone, VMArray<float> pos);
		void destroyBoneSphere(std::uint32_t handle);
		void registerForBoneSphereEvents(VMObject* scriptObj);
		void unRegisterForBoneSphereEvents(VMObject* scriptObj);
		void toggleDebugBoneSpheres(bool turnOn) const;
		void toggleDebugBoneSpheresAtBone(std::uint32_t handle, bool turnOn);

	private:
		static bool registerPapyrusFunctionsCallback(RE::BSScript::Internal::VirtualMachine* vm);
		static std::uint32_t registerBoneSphereFunc(StaticFunctionTag* base, float radius, RE::BSFixedString bone);
		static std::uint32_t registerBoneSphereOffsetFunc(StaticFunctionTag* base, float radius, RE::BSFixedString bone, VMArray<float> pos);
		static void destroyBoneSphereFunc(StaticFunctionTag* base, std::uint32_t handle);
		static void registerForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj);
		static void unRegisterForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj);
		static void toggleDebugBoneSpheresFunc(StaticFunctionTag* base, bool turnOn);
		static void toggleDebugBoneSpheresAtBoneFunc(StaticFunctionTag* base, std::uint32_t handle, bool turnOn);

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
