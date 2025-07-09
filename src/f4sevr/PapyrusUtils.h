#pragma once

#include "PapyrusArgs.h"

namespace F4SEVR
{
    typedef void (*CallGlobalFunctionNoWaitType)(VirtualMachine* vm, std::uint64_t unk1, std::uint64_t unk2, const RE::BSFixedString* className,
        const RE::BSFixedString* eventName, VMValue* args);
    inline REL::Relocation<CallGlobalFunctionNoWaitType> CallGlobalFunctionNoWait_Internal(REL::Offset(0x014D6BD0));

    typedef void (*CallFunctionNoWaitType)(VirtualMachine* vm, std::uint64_t unk1, VMIdentifier* vmIdentifier, const RE::BSFixedString* eventName, VMValue* args);
    inline REL::Relocation<CallFunctionNoWaitType> CallFunctionNoWait_Internal(REL::Offset(0x0145BB20));

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

        auto bsClassName = RE::BSFixedString(className.c_str());
        auto bsFuncName = RE::BSFixedString(funcName.c_str());
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

        auto bsFuncName = RE::BSFixedString(funcName.c_str());
        CallFunctionNoWait_Internal(vm, 0, ident, &bsFuncName, &packedArgs);
    }
}
