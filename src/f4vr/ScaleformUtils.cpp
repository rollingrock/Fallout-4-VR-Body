#include <stdexcept>

#include "scaleformUtils.h"
#include "../common/Logger.h"

/**
 * For more documentation on Scaleform see: https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development-%E2%80%90-Pipboy-Controls
 */
namespace {
	/**
	 * Execute an operation (moveUp, moveDown, select/press) on a generic Scaleform list element.
	 */
	bool invokeOperationOnListElement(GFxMovieRoot* root, GFxValue* list, const f4vr::ScaleformListOp op, const char* listPath) {
		bool success;
		switch (op) {
		case f4vr::ScaleformListOp::MoveUp:
			success = list->Invoke("moveSelectionUp", nullptr, nullptr, 0);
			common::Log::verbose("Move selection up on list:('%s'), success:(%d)", listPath, success);
			return success;
		case f4vr::ScaleformListOp::MoveDown:
			success = list->Invoke("moveSelectionDown", nullptr, nullptr, 0);
			common::Log::verbose("Move selection down on list:('%s'), success:(%d)", listPath, success);
			return success;
		case f4vr::ScaleformListOp::Select:
			GFxValue event;
			GFxValue args[3];
			args[0].SetString("BSScrollingList::itemPress");
			args[1].SetBool(true);
			args[2].SetBool(true);
			root->CreateObject(&event, "flash.events.Event", args, 3);
			success = list->Invoke("dispatchEvent", nullptr, &event, 1);
			common::Log::verbose("Press selection on list:('%s'), success:(%d)", listPath, success);
			return success;
		}
		throw std::invalid_argument("Invalid ScaleformListOp value");
	}
}

namespace f4vr {
	/**
	 * Execute an operation (moveUp, moveDown, select/press) on a Scaleform list that can be found by static path.
	 */
	bool doOperationOnList(GFxMovieRoot* root, const char* listPath, const ScaleformListOp op) {
		GFxValue list;
		if (!root->GetVariable(&list, listPath)) {
			common::Log::debug("List operation failed on list:('%s'), list not found", listPath);
			return false;
		}
		return invokeOperationOnListElement(root, &list, op, listPath);
	}

	/**
	 * Execute an operation (moveUp, moveDown, select/press) on a Scaleform list that is inside a context menu
	 * message box. The message box can be found by static path but the list inside is dynamic and found by traversing the hierarchy.
	 */
	bool doOperationOnMessageHolderList(GFxMovieRoot* root, const char* messageHolderPath, const ScaleformListOp op) {
		GFxValue messageHolder;
		if (!root->GetVariable(&messageHolder, messageHolderPath)) {
			common::Log::debug("List operation failed on message box list:('%s'), message box not found", messageHolderPath);
			return false;
		}
		return findAndWorkOnElement(&messageHolder, "List_mc", [root,op, messageHolderPath](GFxValue& list) {
				invokeOperationOnListElement(root, &list, op, messageHolderPath);
			}
		);
	}

	/**
	 * Find an element by name in the Scaleform hierarchy and perform an action on it.
	 * Note: all my attempts to return the found element for more common API pattern result in the game crashing. I think
	 * there may be an issue with the ref counting, or I'm an idiot.
	 */
	bool findAndWorkOnElement(GFxValue* elm, const std::string& name, const std::function<void(GFxValue&)>& doWork) {
		if (!elm || elm->IsUndefined() || !elm->IsObject())
			return false;

		GFxValue nameVal;
		if (elm->GetMember("name", &nameVal) && nameVal.IsString()) {
			if (nameVal.GetString() == name) {
				doWork(*elm);
				return true;
			}
		}

		GFxValue countVal;
		elm->GetMember("numChildren", &countVal);
		const int count = countVal.GetType() == GFxValue::kType_Int ? countVal.GetInt() : 0;

		for (int i = 0; i < count; ++i) {
			GFxValue child;
			GFxValue args[1];
			args[0].SetInt(i);
			if (elm->Invoke("getChildAt", &child, args, 1)) {
				if (findAndWorkOnElement(&child, name, doWork)) {
					return true; // stop on first match
				}
			}
		}

		return false;
	}
}
