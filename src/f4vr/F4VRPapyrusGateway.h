#pragma once

#include "F4VRUtils.h"
#include "../common/Logger.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusNativeFunctions.h"

namespace f4vr {
	/**
	 * A way to execute Papyrus functions from C++ code.
	 */
	class F4VRPapyrusGateway {
	public:
		virtual ~F4VRPapyrusGateway() { _instance = nullptr; }

		void init(const F4SEInterface* f4se) {
			if (_instance) {
				throw std::exception("Papyrus Gateway is already initialized, only single instance can be used!");
			}
			_instance = this;
			registerPapyrusNativeFunctions(f4se, registerPapyrusFunctionsCallback);
		}

		/**
		 * Enable/disable the player ability to move, fire weapon, use vats, etc.
		 * @param enable if true will enable everything, if false will disable specifics
		 * @param combat only when disabling, if true will disable combat controls (fire weapon)
		 * @param drawWeapon only when enabling, if true will draw equipped weapon
		 */
		void enableDisablePlayerControls(const bool enable, const bool combat, const bool drawWeapon) const {
			auto arguments = getArgs(enable, combat, drawWeapon);
			executePapyrusScript("EnableDisablePlayerControls", arguments);
		}

		/**
		 * Enable/disable that VATS feature in the game.
		 * Disabling will free the "B" button for general use.
		 */
		void enableDisableVats(const bool enable) const {
			// common::Log::warn("Try stuff...");
			// const auto quest = DYNAMIC_CAST(LookupFormByID(0xB4000F9B), TESForm, TESQuest);
			//
			// common::Log::warn("Try stuff 2...");
			// const auto policy = (*g_gameVM)->m_virtualMachine->GetHandlePolicy();
			//
			// common::Log::warn("Try stuff 3...");
			// const auto handle = policy->Create(kFormType_QUST, quest);
			// common::Log::warn("got handle? %lld", handle);
			//
			// common::Log::warn("same handle? %lld", thisObject->GetHandle());
			auto arguments = getArgs(enable);
			executePapyrusScript("EnableDisableVats", arguments);
		}

		/**
		 * Draw the currently equipped weapon if any.
		 */
		void drawWeapon() const {
			executePapyrusScript("DrawWeapon");
		}

		/**
		 * Holster currently drawn weapon if any.
		 */
		void holsterWeapon() const {
			executePapyrusScript("HolsterWeapon");
		}

		/**
		 * Un-equip the currently equipped weapon.
		 * NOT the same as holstering (not available)
		 */
		void UnEquipCurrentWeapon(const bool enable) const {
			executePapyrusScript("UnEquipCurrentWeapon");
		}

	private:
		/**
		 * Register a native papyrus function that is called from a papyrus script to register for gateway access.
		 * I couldn't find a way to get the script object handle so this is a workaround for the papyrus script to send its handle.
		 */
		static bool registerPapyrusFunctionsCallback(VirtualMachine* vm) {
			vm->RegisterFunction(new NativeFunction1("RegisterPapyrusGatewayScript", "FRIK:FRIK", onRegisterGatewayScript, vm));
			return true;
		}

		/**
		 * On receiving a papyrus script registration call, we store the script handle and name for later to call scripts in it.
		 */
		static void onRegisterGatewayScript(StaticFunctionTag* base, VMObject* scriptObj) {
			if (scriptObj && _instance) {
				common::Log::info("Register for Papyrus gateway with Script:'%s' Handle:(%lld)", scriptObj->GetObjectType().c_str(), scriptObj->GetHandle());
				_instance->_scriptHandle = scriptObj->GetHandle();
				_instance->_scriptName = scriptObj->GetObjectType().c_str();
			} else {
				common::Log::error("Papyrus Gateway instance is not set or scriptObj is null");
			}
		}

		/**
		 * No args papyrus function call.
		 */
		void executePapyrusScript(const char* functionName) const {
			VMArray<VMVariable> arguments;
			executePapyrusScript(functionName, arguments);
		}

		/**
		 * Execute the given papyrus function on the registered script object with the given arguments.
		 */
		void executePapyrusScript(const char* functionName, VMArray<VMVariable>& arguments) const {
			const auto vm = (*g_gameVM)->m_virtualMachine;
			VMIdentifier* ident = nullptr;
			if (!vm->GetObjectIdentifier(_scriptHandle, _scriptName.c_str(), 0, &ident, 0)) {
				common::Log::error("Failed to get script identifier for '%s' (%lld)", _scriptName.c_str(), _scriptHandle);
				return;
			}

			VMValue packedArgs;
			arguments.PackArray(&packedArgs, vm);

			common::Log::info("Calling papyrus function '%s' on script '%s'", functionName, _scriptName.c_str());
			const BSFixedString bsFunctionName = functionName;
			CallFunctionNoWait_Internal(vm, 0, ident, &bsFunctionName, &packedArgs);
		}

		/**
		 * Small helper function to add an arguments
		 */
		template <typename T>
		static void addArgument(VMArray<VMVariable>& arguments, T arg) {
			VMVariable var1;
			var1.Set(&arg);
			arguments.Push(&var1);
		}

		template <typename T>
		static VMArray<VMVariable> getArgs(T arg) {
			VMArray<VMVariable> arguments;
			VMVariable var1;
			var1.Set(&arg);
			arguments.Push(&var1);
			return arguments;
		}

		template <typename T1, typename T2>
		static VMArray<VMVariable> getArgs(T1 arg1, T2 arg2) {
			VMArray<VMVariable> arguments;
			VMVariable var1;
			var1.Set(&arg1);
			arguments.Push(&var1);
			VMVariable var2;
			var2.Set(&arg2);
			arguments.Push(&var2);
			return arguments;
		}

		template <typename T1, typename T2, typename T3>
		static VMArray<VMVariable> getArgs(T1 arg1, T2 arg2, T3 arg3) {
			VMArray<VMVariable> arguments;
			VMVariable var1;
			var1.Set(&arg1);
			arguments.Push(&var1);
			VMVariable var2;
			var2.Set(&arg2);
			arguments.Push(&var2);
			VMVariable var3;
			var3.Set(&arg3);
			arguments.Push(&var3);
			return arguments;
		}

		// the handle to the script object to call its papyrus functions
		UINT64 _scriptHandle = 0;

		// the script name (object type) as received from registration call
		std::string _scriptName;

		// workaround as papyrus registration requires global functions.  
		inline static F4VRPapyrusGateway* _instance = nullptr;
	};
}
