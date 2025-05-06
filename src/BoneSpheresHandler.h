#pragma once

#include "f4se/PapyrusEvents.h"

namespace frik {
	constexpr auto BONE_SPHERE_EVEN_NAME = "OnBoneSphereEvent";

	enum class BoneSphereEvent : uint8_t {
		None = 0,
		Enter = 1,
		Exit = 2,
		Holster = 3,
		Draw = 4
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

		BoneSphere(const float a_radius, NiNode* a_bone, const NiPoint3 a_offset)
			: radius(a_radius), bone(a_bone), offset(a_offset) {
			stickyRight = false;
			stickyLeft = false;
			turnOnDebugSpheres = false;
			debugSphere = nullptr;
		}

		float radius;
		NiNode* bone;
		NiPoint3 offset;
		bool stickyRight;
		bool stickyLeft;
		bool turnOnDebugSpheres;
		NiNode* debugSphere;
	};

	class BoneSpheresHandler {
	public:
		void onFrameUpdate();

		void holsterWeapon();
		void drawWeapon();

		UInt32 registerBoneSphere(float radius, BSFixedString bone);
		UInt32 registerBoneSphereOffset(float radius, BSFixedString bone, VMArray<float> pos);
		void destroyBoneSphere(UInt32 handle);
		void registerForBoneSphereEvents(VMObject* thisObject);
		void unRegisterForBoneSphereEvents(VMObject* thisObject);
		void toggleDebugBoneSpheres(bool turnOn) const;
		void toggleDebugBoneSpheresAtBone(UInt32 handle, bool turnOn);

	private:
		void detectBoneSphere();
		void handleDebugBoneSpheres();

		RegistrationSetHolder<NullParameters> _boneSphereEventRegs;

		std::map<UInt32, BoneSphere*> _boneSphereRegisteredObjects;
		UInt32 _nextBoneSphereHandle = 1;
		UInt32 _curDevice = 0;
	};
}
