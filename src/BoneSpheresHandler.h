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

		BoneSphere(const float a_radius, NiNode* a_bone, const RE::NiPoint3 a_offset)
			: radius(a_radius), bone(a_bone), offset(a_offset) {
			stickyRight = false;
			stickyLeft = false;
			turnOnDebugSpheres = false;
			debugSphere = nullptr;
		}

		float radius;
		NiNode* bone;
		RE::NiPoint3 offset;
		bool stickyRight;
		bool stickyLeft;
		bool turnOnDebugSpheres;
		NiNode* debugSphere;
	};

	class BoneSpheresHandler {
	public:
		virtual ~BoneSpheresHandler() { _instance = nullptr; }

		void init(const F4SEInterface* f4se);
		void onFrameUpdate();

		UInt32 registerBoneSphere(float radius, BSFixedString bone);
		UInt32 registerBoneSphereOffset(float radius, BSFixedString bone, VMArray<float> pos);
		void destroyBoneSphere(UInt32 handle);
		void registerForBoneSphereEvents(VMObject* scriptObj);
		void unRegisterForBoneSphereEvents(VMObject* scriptObj);
		void toggleDebugBoneSpheres(bool turnOn) const;
		void toggleDebugBoneSpheresAtBone(UInt32 handle, bool turnOn);

	private:
		static bool registerPapyrusFunctionsCallback(VirtualMachine* vm);
		static UInt32 registerBoneSphereFunc(StaticFunctionTag* base, float radius, BSFixedString bone);
		static UInt32 registerBoneSphereOffsetFunc(StaticFunctionTag* base, float radius, BSFixedString bone, VMArray<float> pos);
		static void destroyBoneSphereFunc(StaticFunctionTag* base, UInt32 handle);
		static void registerForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj);
		static void unRegisterForBoneSphereEventsFunc(StaticFunctionTag* base, VMObject* scriptObj);
		static void toggleDebugBoneSpheresFunc(StaticFunctionTag* base, bool turnOn);
		static void toggleDebugBoneSpheresAtBoneFunc(StaticFunctionTag* base, UInt32 handle, bool turnOn);

		void detectBoneSphere();
		void handleDebugBoneSpheres();

		RegistrationSetHolder<> _boneSphereEventRegs;

		std::map<UInt32, BoneSphere*> _boneSphereRegisteredObjects;
		UInt32 _nextBoneSphereHandle = 1;
		UInt32 _curDevice = 0;

		// workaround as papyrus registration requires global functions.
		inline static BoneSpheresHandler* _instance = nullptr;
	};
}
