#pragma once

#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"

namespace f4vr {
	constexpr UInt32 PipboyAA = 0x0001ED3D;
	constexpr UInt32 PipboyArmor = 0x00021B3B;
	constexpr UInt32 MiningHelmet = 0x0022DD1F;
	constexpr UInt32 PALightKW = 0x000B34A6;

	inline RelocAddr<uintptr_t> NEW_REFR_DATA_VTABLE(0x2c8d080);

	using _BGSObjectInstance_CTOR = void* (*)(void* instance, TESForm* a_form, TBO_InstanceData* a_instanceData);
	inline RelocAddr<_BGSObjectInstance_CTOR> BGSObjectInstance_CTOR(0x2dd930);

	// adapted from Commonlib
	class __declspec(novtable) NEW_REFR_DATA {
	public:
		NEW_REFR_DATA() {
			vtable = NEW_REFR_DATA_VTABLE;
		}

		// members
		uintptr_t vtable; // 00
		RE::NiPoint3 location; // 08
		RE::NiPoint3 direction; // 14
		TESBoundObject* object{nullptr}; // 20
		TESObjectCELL* interior{nullptr}; // 28
		TESWorldSpace* world{nullptr}; // 30
		TESObjectREFR* reference{nullptr}; // 38
		void* addPrimitive{nullptr}; // 40
		void* additionalData{nullptr}; // 48
		ExtraDataList* extra{nullptr}; // 50
		void* instanceFilter{nullptr}; // 58
		BGSObjectInstanceExtra* modExtra{nullptr}; // 60
		std::uint16_t maxLevel{0}; // 68
		bool forcePersist{false}; // 6A
		bool clearStillLoadingFlag{false}; // 6B
		bool initializeScripts{true}; // 6C
		bool initiallyDisabled{false}; // 6D
	};

	static_assert(sizeof(NEW_REFR_DATA) == 0x70);

	class BGSEquipIndex {
	public:
		~BGSEquipIndex() noexcept {} // NOLINT(modernize-use-equals-default)

		// members
		std::uint32_t index; // 0
	};

	static_assert(sizeof(BGSEquipIndex) == 0x4);

	class BGSObjectInstance {
	public:
		BGSObjectInstance(TESForm* a_object, TBO_InstanceData* a_instanceData) {
			ctor(a_object, a_instanceData);
		}

		// members
		TESForm* object{nullptr}; // 00
		TBO_InstanceData* instanceData; // 08

	private:
		BGSObjectInstance* ctor(TESForm* object, TBO_InstanceData* instanceData) {
			return static_cast<BGSObjectInstance*>(BGSObjectInstance_CTOR(static_cast<void*>(this), object, instanceData));
		}
	};

	static_assert(sizeof(BGSObjectInstance) == 0x10);

	class hknpMotionPropertiesId {
	public:
		enum Preset {
			STATIC = 0, ///< No velocity allowed
			DYNAMIC, ///< For regular dynamic bodies, undamped and gravity factor = 1
			KEYFRAMED, ///< like DYNAMIC, but gravity factor = 0
			FROZEN, ///< like KEYFRAMED, but lots of damping
			DEBRIS, ///< like DYNAMIC, but aggressive deactivation

			NUM_PRESETS
		};
	};

	enum BIPED_SLOTS {
		slot_None = 0,
		slot_HairTop = 1 << 0,
		slot_HairLong = 1 << 1,
		slot_Head = 1 << 2,
		slot_Body = 1 << 3,
		slot_LHand = 1 << 4,
		slot_RHand = 1 << 5,
		slot_UTorso = 1 << 6,
		slot_ULArm = 1 << 7,
		slot_URArm = 1 << 8,
		slot_ULLeg = 1 << 9,
		slot_URLeg = 1 << 10,
		slot_ATorso = 1 << 11,
		slot_ALArm = 1 << 12,
		slot_ARArm = 1 << 13,
		slot_ALLeg = 1 << 14,
		slot_ARLeg = 1 << 15,
		slot_Headband = 1 << 16,
		slot_Eyes = 1 << 17,
		slot_Beard = 1 << 18,
		slot_Mouth = 1 << 19,
		slot_Neck = 1 << 20,
		slot_Ring = 1 << 21,
		slot_Scalp = 1 << 22,
		slot_Decapitation = 1 << 23,
		slot_Unnamed1 = 1 << 24,
		slot_Unnamed2 = 1 << 25,
		slot_Unnamed3 = 1 << 26,
		slot_Unnamed4 = 1 << 27,
		slot_Unnamed5 = 1 << 28,
		slot_Shield = 1 << 29,
		slot_Pipboy = 1 << 30,
		slot_FX = 1 << 31
	};

	enum class WeaponType : std::uint8_t {
		HandToHandMelee,
		OneHandSword,
		OneHandDagger,
		OneHandAxe,
		OneHandMace,
		TwoHandSword,
		TwoHandAxe,
		Bow,
		Staff,
		Gun,
		Grenade,
		Mine,
		Spell,
		Shield,
		Torch
	};

	struct NiCloneProcess {
		UInt64 unk00 = 0;
		UInt64 unk08 = 0; // Start of clone list 1?
		UInt64 unk10 = 0;
		UInt64* unk18; // initd to RelocAddr(0x36ff560)
		UInt64 unk20 = 0;
		UInt64 unk28 = 0;
		UInt64 unk30 = 0;
		UInt64 unk38 = 0; // Start of clone list 2?
		UInt64 unk40 = 0;
		UInt64* unk48; // initd to RelocAddr(0x36ff564)
		UInt64 unk50 = 0;
		UInt64 unk58 = 0;
		UInt8 copyType = 1; // 60 - CopyType - default 1
		UInt8 m_eAffectedNodeRelationBehavior = 0; // 61 - CloneRelationBehavior - default 0
		UInt8 m_eDynamicEffectRelationBehavior = 0; // 62 - CloneRelationBehavior - default 0
		char m_cAppendChar = '$'; // 64 - default '$'
		RE::NiPoint3 scale = {1.0f, 1.0f, 1.0f}; // 0x68 - default {1, 1, 1}
	};

	class MuzzleFlash {
	public:
		uint64_t unk00;
		uint64_t unk08;
		NiNode* fireNode;
		NiNode* projectileNode;
	};
}
