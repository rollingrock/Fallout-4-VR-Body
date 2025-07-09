#pragma once

#include "PapyrusArgs.h"

namespace F4SEVR
{
    // This is the callback function to ScriptObject.SendCustomEvent, the high-level parameters were more convenient
    // The only issue is you actually need a sending object and a CustomEvent on the sender's script, which can't be native
    typedef void (*_SendCustomEvent)(VirtualMachine* vm, UInt64 unk1, VMIdentifier* sender, const BSFixedString* eventName, VMValue* args);
    inline REL::Relocation<_SendCustomEvent> SendCustomEvent_Internal(REL::Offset(0x0145E5B0));

    typedef void (*CallGlobalFunctionNoWaitType)(VirtualMachine* vm, std::uint64_t unk1, std::uint64_t unk2, const BSFixedString* className,
        const BSFixedString* eventName, VMValue* args);
    inline REL::Relocation<CallGlobalFunctionNoWaitType> CallGlobalFunctionNoWait_Internal(REL::Offset(0x014D6BD0));

    typedef void (*CallFunctionNoWaitType)(VirtualMachine* vm, std::uint64_t unk1, VMIdentifier* vmIdentifier, const BSFixedString* eventName, VMValue* args);
    inline REL::Relocation<CallFunctionNoWaitType> CallFunctionNoWait_Internal(REL::Offset(0x0145BB20));

    /**
     * Small helper function to add an arguments
     */
    template <typename T>
    static void addArgument(VMArray<VMVariable>& arguments, T arg)
    {
        VMVariable var1;
        var1.Set(&arg);
        arguments.Push(&var1);
    }

    template <typename T>
    static VMArray<VMVariable> getArgs(T arg)
    {
        VMArray<VMVariable> arguments;
        VMVariable var1;
        var1.Set(&arg);
        arguments.Push(&var1);
        return arguments;
    }

    template <typename T1, typename T2>
    static VMArray<VMVariable> getArgs(T1 arg1, T2 arg2)
    {
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
    static VMArray<VMVariable> getArgs(T1 arg1, T2 arg2, T3 arg3)
    {
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
    static VMArray<VMVariable> getArgs(T1 arg1, T2 arg2, T3 arg3, T4 arg4)
    {
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

    template <typename T>
    void execPapyrusGlobalFunction(const std::string& className, const std::string& funcName, T arg1)
    {
        auto vm = getGameVM()->m_virtualMachine;

        VMArray<VMVariable> arguments;
        VMVariable var1;
        var1.Set<T>(&arg1);
        arguments.Push(&var1);

        VMValue args;
        PackValue(&args, &arguments, vm);

        auto bsClassName = BSFixedString(className.c_str());
        auto bsFuncName = BSFixedString(funcName.c_str());
        CallGlobalFunctionNoWait_Internal(vm, 0, 0, &bsClassName, &bsFuncName, &args);
    }

    inline void execPapyrusGlobalFunction(const std::string& className, const std::string& funcName, const std::string& text)
    {
        const BSFixedString bsFixedString(text.c_str());
        execPapyrusGlobalFunction(className, funcName, bsFixedString);
    }

    inline void execPapyrusFunction(const std::uint64_t scriptHandle, const std::string& scriptName, const std::string& funcName, VMArray<VMVariable>& arguments)
    {
        auto vm = getGameVM()->m_virtualMachine;

        VMIdentifier* ident = nullptr;
        if (!vm->GetObjectIdentifier(scriptHandle, scriptName.c_str(), 0, &ident, 0)) {
            common::logger::error("Failed to get script identifier for '{}' ({})", scriptName.c_str(), scriptHandle);
            return;
        }

        VMValue packedArgs;
        arguments.PackArray(&packedArgs, vm);

        auto bsFuncName = BSFixedString(funcName.c_str());
        CallFunctionNoWait_Internal(vm, 0, ident, &bsFuncName, &packedArgs);
    }
}
