// SPDX-License-Identifier: MIT
// MIT-licensed (see DevBenchAPI.LICENSE.txt) so any plugin may vendor it; the devbench
// plugin itself is GPL-3.0. Compile this in YOUR plugin only (not in devbench).
//
// THE ONLY EXTENDER-AWARE FILE IN THE ABI. DevBenchAPI.h is plain C++; everything
// game-specific is the messaging dispatch below, which is the same call on both
// extenders — SKSE and F4SE both expose
//     GetMessagingInterface()->Dispatch(type, data, dataLen, receiver)
// with identical semantics, so the only real difference is which header to include.
//
// Game selection, in order:
//   1. Define DEVBENCHAPI_GAME_SKYRIM or DEVBENCHAPI_GAME_FALLOUT4 to force it.
//   2. Otherwise it is auto-detected from which extender header is reachable.
// The explicit macro exists because auto-detection is a convenience, not a guarantee:
// a plugin whose include path can see both headers would otherwise get whichever this
// file happens to test first, silently.
#include "DevBenchAPI.h"

#if !defined(DEVBENCHAPI_GAME_SKYRIM) && !defined(DEVBENCHAPI_GAME_FALLOUT4)
#	if __has_include(<F4SE/F4SE.h>)
#		define DEVBENCHAPI_GAME_FALLOUT4 1
#	elif __has_include(<SKSE/SKSE.h>)
#		define DEVBENCHAPI_GAME_SKYRIM 1
#	else
#		error "DevBenchAPI.cpp: no script-extender header found. Define DEVBENCHAPI_GAME_SKYRIM or DEVBENCHAPI_GAME_FALLOUT4, or add SKSE/F4SE to your include path."
#	endif
#endif

#if defined(DEVBENCHAPI_GAME_FALLOUT4)
#	include <F4SE/F4SE.h>
// The load-order fallback below calls the raw Win32 loader API. CommonLibF4 deliberately
// does not pull in Windows.h, so a consumer without it in a PCH would fail to compile.
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <Windows.h>
#else
#	include <SKSE/SKSE.h>
#endif

// Consumer-side helper — compile this in YOUR plugin (devbench itself does not build it).
DevBenchAPI::IDevBenchInterface001* g_devBenchInterface = nullptr;

namespace DevBenchAPI
{
	IDevBenchInterface001* GetDevBenchInterface001()
	{
		if (g_devBenchInterface)
			return g_devBenchInterface;

#if defined(DEVBENCHAPI_GAME_FALLOUT4)
		const auto messaging = F4SE::GetMessagingInterface();
#else
		const auto messaging = SKSE::GetMessagingInterface();
#endif
		if (!messaging)
			return nullptr;

		// Synchronous: dispatching to the named provider invokes its listener inline,
		// which fills message.GetApiFunction in this stack struct.
		//
		// NOTE the dataLen argument is sizeof(DevBenchMessage*), not sizeof(DevBenchMessage).
		// That is almost certainly a slip in the original, but it is the value every
		// already-shipped consumer sends and the host has never read it — so it stays as
		// published. Do not "fix" it into a value the host might one day start validating.
		DevBenchMessage message;
		messaging->Dispatch(DevBenchMessage::kMessage_GetInterface, &message,
			sizeof(DevBenchMessage*), DevBenchPluginName);
		if (!message.GetApiFunction) {
#if defined(DEVBENCHAPI_GAME_FALLOUT4)
			// FALLBACK (2026-08-26, FO4VR): the messaging handshake requires the
			// provider's any-sender listener to be present in THIS plugin's
			// listener slot, and F4SEVR's RegisterListener de-dupes by handle -
			// a provider that loaded earlier can be unreachable by message
			// forever (its kPostLoad re-register is a global no-op). The DLL
			// export is load-order-proof; same GetApi either way.
			if (const auto mod = ::GetModuleHandleW(L"devbench.dll")) {
				if (const auto entry = reinterpret_cast<void* (*)()>(
						::GetProcAddress(mod, "DevBench_GetApiFunction"))) {
					message.GetApiFunction =
						reinterpret_cast<decltype(message.GetApiFunction)>(entry());
				}
			}
#endif
		}
		if (!message.GetApiFunction)
			return nullptr;


		g_devBenchInterface = static_cast<IDevBenchInterface001*>(message.GetApiFunction(1));
		return g_devBenchInterface;
	}
}
