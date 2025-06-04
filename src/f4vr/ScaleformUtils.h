#pragma once

#include <functional>
#include <f4se/ScaleformMovie.h>
#include <f4se/ScaleformValue.h>

namespace f4vr {
	enum class ScaleformListOp : UInt8 {
		MoveUp = 0,
		MoveDown,
		Select,
	};

	bool doOperationOnList(GFxMovieRoot* root, const char* listPath, ScaleformListOp op);
	bool doOperationOnMessageHolderList(GFxMovieRoot* root, const char* messageHolderPath, ScaleformListOp op);

	bool findAndWorkOnElement(GFxValue* elm, const std::string& name, const std::function<void(GFxValue&)>& doWork);
}
