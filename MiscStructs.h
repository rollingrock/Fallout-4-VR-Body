#pragma once
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"

namespace F4VRBody {

	extern RelocAddr<uintptr_t> NEW_REFR_DATA_VTABLE;
	// adapted from Commonlib
	class __declspec(novtable) NEW_REFR_DATA
	{
	public:
		NEW_REFR_DATA()
		{
			vtable = NEW_REFR_DATA_VTABLE;
		}

		// members
		uintptr_t vtable;								  // 00
		NiPoint3 location;                                // 08
		NiPoint3 direction;                               // 14
		TESBoundObject* object{ nullptr };                // 20
		TESObjectCELL* interior{ nullptr };               // 28
		TESWorldSpace* world{ nullptr };                  // 30
		TESObjectREFR* reference{ nullptr };              // 38
		void* addPrimitive{ nullptr };            // 40
		void* additionalData{ nullptr };                  // 48
		ExtraDataList* extra{ nullptr };  // 50
		void* instanceFilter{ nullptr };       // 58
		BGSObjectInstanceExtra* modExtra{ nullptr };      // 60
		std::uint16_t maxLevel{ 0 };                      // 68
		bool forcePersist{ false };                       // 6A
		bool clearStillLoadingFlag{ false };              // 6B
		bool initializeScripts{ true };                   // 6C
		bool initiallyDisabled{ false };                  // 6D
	};
	static_assert(sizeof(NEW_REFR_DATA) == 0x70);


	class BGSEquipIndex
	{
	public:
		~BGSEquipIndex() noexcept {}  // NOLINT(modernize-use-equals-default)

		// members
		std::uint32_t index;  // 0
	};
	static_assert(sizeof(BGSEquipIndex) == 0x4);

	typedef void*(*_BGSObjectInstance_CTOR)(void* instance, TESForm* a_form, TBO_InstanceData* a_instanceData);
	extern RelocAddr<_BGSObjectInstance_CTOR> BGSObjectInstance_CTOR;

	class BGSObjectInstance
	{
	public:
		BGSObjectInstance(TESForm* a_object, TBO_InstanceData* a_instanceData)
		{
			ctor(a_object, a_instanceData);
		}

		// members
		TESForm* object{ nullptr };                      // 00
		TBO_InstanceData* instanceData;  // 08

	private:
		BGSObjectInstance* ctor(TESForm* a_object, TBO_InstanceData* a_instanceData)
		{
			return (BGSObjectInstance*)BGSObjectInstance_CTOR((void*)this, a_object, a_instanceData);
		}
	};
	static_assert(sizeof(BGSObjectInstance) == 0x10);

	class hknpMotionPropertiesId
	{
	public:
		enum Preset
		{
			STATIC = 0,  ///< No velocity allowed
			DYNAMIC,     ///< For regular dynamic bodies, undamped and gravity factor = 1
			KEYFRAMED,   ///< like DYNAMIC, but gravity factor = 0
			FROZEN,      ///< like KEYFRAMED, but lots of damping
			DEBRIS,      ///< like DYNAMIC, but aggressive deactivation

			NUM_PRESETS
		};
	};
}
