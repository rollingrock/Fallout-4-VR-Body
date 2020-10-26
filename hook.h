#pragma once

#include "f4se/PluginAPI.h"  // SKSEInterface, PluginInfo
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"
#include "F4SE_common/BranchTrampoline.h"


#include "xbyak/xbyak.h"


void hookMain();