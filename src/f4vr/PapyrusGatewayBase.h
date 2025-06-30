#pragma once

#include "F4VRUtils.h"
#include "../common/Logger.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusNativeFunctions.h"

namespace f4vr {
	/**
	 * A way to execute Papyrus functions from C++ code.
	 * Extend to create a custom gateway for your mod.
	 * Only one instance of this class can be used.
	 *
	 * Requires 2 papyrus scripts in the plugging. One with a native "RegisterPapyrusGatewayScript" function that is called by
	 * the second script to register itself for the gateway access. The second script contains the actual functions that can be called from C++ code.
	 *
	 */
	class PapyrusGatewayBase {
	public:
		explicit PapyrusGatewayBase(const F4SEInterface* f4se, std::string registerScriptClassName)
			: _registerScriptClassName(std::move(registerScriptClassName)) {
			if (_instance && _instance != this) {
				throw std::exception("Papyrus Gateway is already initialized, only single instance can be used!");
			}
			_instance = this;
			registerPapyrusNativeFunctions(f4se, registerPapyrusFunctionsCallback);

			// TODO: can I get the script handle without register function?
			//
			// common::logger::warn("Try stuff...");
			// const auto quest = DYNAMIC_CAST(LookupFormByID(0xB4000F9B), TESForm, TESQuest);
			//
			// common::logger::warn("Try stuff 2...");
			// const auto policy = (*g_gameVM)->m_virtualMachine->GetHandlePolicy();
			//
			// common::logger::warn("Try stuff 3...");
			// const auto handle = policy->Create(kFormType_QUST, quest);
			// common::logger::warn("got handle? {}", handle);
			//
			// common::logger::warn("same handle? {}", thisObject->GetHandle());
		}

		virtual ~PapyrusGatewayBase() { _instance = nullptr; }

	protected:
		/**
		 * Register a native papyrus function that is called from a papyrus script to register for gateway access.
		 * I couldn't find a way to get the script object handle so this is a workaround for the papyrus script to send its handle.
		 */
		static bool registerPapyrusFunctionsCallback(VirtualMachine* vm) {
			vm->RegisterFunction(new NativeFunction1("RegisterPapyrusGatewayScript", _instance->_registerScriptClassName.c_str(), onRegisterGatewayScript, vm));
			return true;
		}

		/**
		 * On receiving a papyrus script registration call, we store the script handle and name for later to call scripts in it.
		 */
		static void onRegisterGatewayScript(StaticFunctionTag* base, VMObject* scriptObj) {
			if (scriptObj && _instance) {
				common::logger::info("Register for Papyrus gateway by Script:'{}' Handle:({})", scriptObj->GetObjectType().c_str(), scriptObj->GetHandle());
				_instance->_scriptHandle = scriptObj->GetHandle();
				_instance->_scriptName = scriptObj->GetObjectType().c_str();
			} else {
				common::logger::error("Papyrus Gateway instance is not set or scriptObj is null");
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
			if (_scriptHandle == 0) {
				common::logger::error("No registered gateway script handle found, Papyrus script missing?");
				return;
			}
			if (!vm->GetObjectIdentifier(_scriptHandle, _scriptName.c_str(), 0, &ident, 0)) {
				common::logger::error("Failed to get script identifier for '{}' ({})", _scriptName.c_str(), _scriptHandle);
				return;
			}

			VMValue packedArgs;
			arguments.PackArray(&packedArgs, vm);

			common::logger::debug("Calling papyrus function '{}' on script '{}'", functionName, _scriptName.c_str());
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

		template <typename T1, typename T2, typename T3, typename T4>
		static VMArray<VMVariable> getArgs(T1 arg1, T2 arg2, T3 arg3, T4 arg4) {
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
			VMVariable var4;
			var4.Set(&arg4);
			arguments.Push(&var4);
			return arguments;
		}

		std::string _registerScriptClassName;

		// the handle to the script object to call its papyrus functions
		UINT64 _scriptHandle = 0;

		// the script name (object type) as received from registration call
		std::string _scriptName;

		// workaround as papyrus registration requires global functions.
		inline static PapyrusGatewayBase* _instance = nullptr;
	};
}
