#pragma once

#include "Skeleton.h"
#include "weaponOffset.h"

namespace F4VRBody {

	class Pipboy	{
	public:
		Pipboy(Skeleton* skelly) {
			playerSkelly = skelly;
		}

		void Pipboy::replaceMeshes(bool force);
			
	private:
		Skeleton* playerSkelly;
		bool meshesReplaced = false;

		void Pipboy::replaceMeshes(std::string itemHide, std::string itemShow);
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern Pipboy* g_pipboy;
}