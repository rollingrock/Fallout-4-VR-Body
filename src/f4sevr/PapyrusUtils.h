#pragma once

#include "PapyrusArgs.h"

namespace F4SEVR
{
    typedef void (*CallGlobalFunctionNoWaitType)(VirtualMachine* vm, std::uint64_t unk1, std::uint64_t unk2, const RE::BSFixedString* className,
        const RE::BSFixedString* eventName, VMValue* args);
    inline REL::Relocation<CallGlobalFunctionNoWaitType> CallGlobalFunctionNoWait_Internal(REL::Offset(0x014D6BD0));

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
}
