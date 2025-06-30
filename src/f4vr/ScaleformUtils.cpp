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
			common::logger::debug("Move selection up on list:('{}'), success:({})", listPath, success);
			return success;
		case f4vr::ScaleformListOp::MoveDown:
			success = list->Invoke("moveSelectionDown", nullptr, nullptr, 0);
			common::logger::debug("Move selection down on list:('{}'), success:({})", listPath, success);
			return success;
		case f4vr::ScaleformListOp::Select:
			GFxValue event;
			GFxValue args[3];
			args[0].SetString("BSScrollingList::itemPress");
			args[1].SetBool(true);
			args[2].SetBool(true);
			root->CreateObject(&event, "flash.events.Event", args, 3);
			success = list->Invoke("dispatchEvent", nullptr, &event, 1);
			common::logger::debug("Press selection on list:('{}'), success:({})", listPath, success);
			return success;
		}
		throw std::invalid_argument("Invalid ScaleformListOp value");
	}
}

namespace f4vr {
	bool getScaleformBool(const GFxMovieRoot* root, const char* path) {
		GFxValue result;
		return root->GetVariable(&result, path) && result.IsBool() && result.GetBool();
	}

	std::optional<int> getScaleformInt(const GFxMovieRoot* root, const char* path) {
		GFxValue result;
		if (!root->GetVariable(&result, path))
			return std::nullopt;
		const auto type = result.GetType();
		if (type == GFxValue::kType_Int || type == GFxValue::kType_UInt) {
			return result.GetInt();
		}
		return std::nullopt;
	}

	bool isElementVisible(const GFxMovieRoot* root, const std::string& path) {
		GFxValue var;
		return root->GetVariable(&var, (path + ".visible").c_str()) && var.IsBool() && var.GetBool();
	}

	/**
	 * Execute an operation (moveUp, moveDown, select/press) on a Scaleform list that can be found by static path.
	 */
	bool doOperationOnScaleformList(GFxMovieRoot* root, const char* listPath, const ScaleformListOp op) {
		GFxValue list;
		if (!root->GetVariable(&list, listPath)) {
			common::logger::trace("List operation failed on list:('{}'), list not found", listPath);
			return false;
		}
		return invokeOperationOnListElement(root, &list, op, listPath);
	}

	/**
	 * Execute an operation (moveUp, moveDown, select/press) on a Scaleform list that is inside a context menu
	 * message box. The message box can be found by static path but the list inside is dynamic and found by traversing the hierarchy.
	 */
	bool doOperationOnScaleformMessageHolderList(GFxMovieRoot* root, const char* messageHolderPath, const ScaleformListOp op) {
		GFxValue messageHolder;
		if (!root->GetVariable(&messageHolder, messageHolderPath)) {
			common::logger::trace("List operation failed on message box list:('{}'), message box not found", messageHolderPath);
			return false;
		}
		return findAndWorkOnScaleformElement(&messageHolder, "List_mc", [root,op, messageHolderPath](GFxValue& list) {
				invokeOperationOnListElement(root, &list, op, messageHolderPath);
			}
		);
	}

	/**
	 * Invoke "ProcessUserEvent" function on the given Scaleform element found by path.
	 * Commonly available for general UI interactions.
	 */
	void invokeScaleformProcessUserEvent(GFxMovieRoot* root, const std::string& path, const char* eventName) {
		GFxValue args[2];
		args[0].SetString(eventName);
		args[1].SetBool(false);
		GFxValue result;
		if (!root->Invoke((path + ".ProcessUserEvent").c_str(), &result, args, 2)) {
			common::logger::warn("Failed to invoke Scaleform ProcessUserEvent '{}' on '{}'", eventName, path.c_str());
		}
		common::logger::debug("Scaleform ProcessUserEvent invoked with '{}' on '{}'; Result:({})", eventName, path.c_str(), result.IsBool() ? result.GetBool() : -1);
	}

	/**
	 * Invoke "dispatchEvent" function on the given Scaleform element found by path.
	 * A framework level events that can be used to trigger code.
	 */
	void invokeScaleformDispatchEvent(GFxMovieRoot* root, const std::string& path, const char* eventName) {
		GFxValue event;
		GFxValue args[3];
		args[0].SetString(eventName);
		args[1].SetBool(true);
		args[2].SetBool(true);
		root->CreateObject(&event, "flash.events.Event", args, 3);
		GFxValue result;
		if (!root->Invoke((path + ".dispatchEvent").c_str(), &result, &event, 1)) {
			common::logger::warn("Failed to invoke Scaleform dispatchEvent '{}' on '{}'", eventName, path.c_str());
		}
		common::logger::debug("Scaleform dispatchEvent invoked with '{}' on '{}'; Result:({})", eventName, path.c_str(), result.IsBool() ? result.GetBool() : -1);
	}

	/**
	 * Find an element by name in the Scaleform hierarchy and perform an action on it.
	 * Note: all my attempts to return the found element for more common API pattern result in the game crashing. I think
	 * there may be an issue with the ref counting, or I'm an idiot.
	 */
	bool findAndWorkOnScaleformElement(GFxValue* elm, const std::string& name, const std::function<void(GFxValue&)>& doWork) {
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
				if (findAndWorkOnScaleformElement(&child, name, doWork)) {
					return true; // stop on first match
				}
			}
		}

		return false;
	}
}
