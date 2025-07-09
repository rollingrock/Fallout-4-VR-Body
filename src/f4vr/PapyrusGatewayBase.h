#pragma once

#include "common/Logger.h"
#include "f4sevr/PapyrusArgs.h"
#include "f4sevr/PapyrusNativeFunctions.h"
#include "f4sevr/PapyrusUtils.h"
#include "f4sevr/PapyrusVM.h"

namespace f4vr
{
    /**
     * A way to execute Papyrus functions from C++ code.
     * Extend to create a custom gateway for your mod.
     * Only one instance of this class can be used.
     *
     * Requires 2 papyrus scripts in the plugging. One with a native "RegisterPapyrusGatewayScript" function that is called by
     * the second script to register itself for the gateway access. The second script contains the actual functions that can be called from C++ code.
     *
     */
    class PapyrusGatewayBase
    {
    public:
        explicit PapyrusGatewayBase(const std::string& registerScriptClassName) :
            _registerScriptClassName(registerScriptClassName)
        {
            if (_instance && _instance != this) {
                throw std::exception("Papyrus Gateway is already initialized, only single instance can be used!");
            }
            _instance = this;

            // Register a native papyrus function that is called from a papyrus script to register for gateway access.
            // I couldn't find a way to get the script object handle so this is a workaround for the papyrus script to send its handle.
            auto vm = F4SEVR::getGameVM()->m_virtualMachine;
            vm->RegisterFunction(new F4SEVR::NativeFunction1("RegisterPapyrusGatewayScript", _instance->_registerScriptClassName.c_str(), onRegisterGatewayScript, vm));
        }

        virtual ~PapyrusGatewayBase() { _instance = nullptr; }

    protected:
        /**
         * On receiving a papyrus script registration call, we store the script handle and name for later to call scripts in it.
         */
        static void onRegisterGatewayScript(F4SEVR::StaticFunctionTag* base, F4SEVR::VMObject* scriptObj)
        {
            if (scriptObj && _instance) {
                const auto scriptName = scriptObj->GetObjectType();
                uint64_t scriptHandle = scriptObj->GetHandle();
                common::logger::info("Register for Papyrus gateway by Script:'{}' Handle:({})", scriptName.c_str(), scriptHandle);
                _instance->_scriptHandle = scriptHandle;
                _instance->_scriptName = scriptName.c_str();
            } else {
                common::logger::error("Papyrus Gateway instance is not set or scriptObj is null");
            }
        }

        /**
         * No args papyrus function call.
         */
        void executePapyrusScript(const char* functionName) const
        {
            F4SEVR::VMArray<F4SEVR::VMVariable> arguments;
            executePapyrusScript(functionName, arguments);
        }

        /**
         * Execute the given papyrus function on the registered script object with the given arguments.
         */
        void executePapyrusScript(const char* functionName, F4SEVR::VMArray<F4SEVR::VMVariable>& arguments) const
        {
            if (_scriptHandle == 0) {
                common::logger::error("No registered gateway script handle found, Papyrus script missing?");
                return;
            }

            common::logger::debug("Calling papyrus function '{}' on script '{}'", functionName, _scriptName.c_str());
            F4SEVR::execPapyrusFunction(_scriptHandle, _scriptName, functionName, arguments);
        }

        /**
         * Small helper function to add an arguments
         */
        template <typename T>
        static void addArgument(F4SEVR::VMArray<F4SEVR::VMVariable>& arguments, T arg)
        {
            F4SEVR::VMVariable var1;
            var1.Set(&arg);
            arguments.Push(&var1);
        }

        template <typename T>
        static F4SEVR::VMArray<F4SEVR::VMVariable> getArgs(T arg)
        {
            F4SEVR::VMArray<F4SEVR::VMVariable> arguments;
            F4SEVR::VMVariable var1;
            var1.Set(&arg);
            arguments.Push(&var1);
            return arguments;
        }

        template <typename T1, typename T2>
        static F4SEVR::VMArray<F4SEVR::VMVariable> getArgs(T1 arg1, T2 arg2)
        {
            F4SEVR::VMArray<F4SEVR::VMVariable> arguments;
            F4SEVR::VMVariable var1;
            var1.Set(&arg1);
            arguments.Push(&var1);
            F4SEVR::VMVariable var2;
            var2.Set(&arg2);
            arguments.Push(&var2);
            return arguments;
        }

        template <typename T1, typename T2, typename T3>
        static F4SEVR::VMArray<F4SEVR::VMVariable> getArgs(T1 arg1, T2 arg2, T3 arg3)
        {
            F4SEVR::VMArray<F4SEVR::VMVariable> arguments;
            F4SEVR::VMVariable var1;
            var1.Set(&arg1);
            arguments.Push(&var1);
            F4SEVR::VMVariable var2;
            var2.Set(&arg2);
            arguments.Push(&var2);
            F4SEVR::VMVariable var3;
            var3.Set(&arg3);
            arguments.Push(&var3);
            return arguments;
        }

        template <typename T1, typename T2, typename T3, typename T4>
        static F4SEVR::VMArray<F4SEVR::VMVariable> getArgs(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
        {
            F4SEVR::VMArray<F4SEVR::VMVariable> arguments;
            F4SEVR::VMVariable var1;
            var1.Set(&arg1);
            arguments.Push(&var1);
            F4SEVR::VMVariable var2;
            var2.Set(&arg2);
            arguments.Push(&var2);
            F4SEVR::VMVariable var3;
            var3.Set(&arg3);
            arguments.Push(&var3);
            F4SEVR::VMVariable var4;
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
