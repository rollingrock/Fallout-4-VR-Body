#include "UIManager.h"
#include "../Config.h"

namespace ui {
	// globals, globals everywhere...
	UIManager* g_uiManager;

	/**
	 * Run frame update on all the containers.
	 */
	void UIManager::onFrameUpdate(UIModAdapter* adapter) {
		_releaseSafeList.clear();

		for (const auto& element : _rootElements) {
			element->onLayoutUpdate(adapter);
		}

		for (const auto& element : _rootElements) {
			element->onFrameUpdate(adapter);
		}

		if (F4VRBody::g_config->checkDebugDumpDataOnceFor("ui_tree")) {
			dumpUITree();
		}
	}

	/**
	 * Attach the given element and subtree to the given attach game node to be rendered and layout with relation to the node.
	 */
	void UIManager::attachElement(const std::shared_ptr<UIElement>& element, NiNode* attachNode) {
		element->attachToNode(attachNode);
		// only the root can exists in the manager collection
		if (!element->getParent()) {
			_MESSAGE("UI Manager root element added and attached to '%s'", attachNode->m_name.c_str());
			_rootElements.emplace_back(element);
		}
	}

	/**
	 * Attach the UI on top of the primary hand and bound to the hand movement.
	 */
	void UIManager::attachPresetToPrimaryWandTop(const std::shared_ptr<UIElement>& element, const float zOffset) {
		element->setPosition(0, 0, 5 + zOffset);
		attachElement(element, getPlayerNodes()->primaryUIAttachNode);
	}

	/**
	 * Attach the UI on left of the primary hand and bound to the hand movement.
	 */
	void UIManager::attachPresetToPrimaryWandLeft(const std::shared_ptr<UIElement>& element, const NiPoint3 offset) {
		element->setPosition(offset.x - 15, offset.y, offset.z - 5);
		attachElement(element, findNode("world_primaryWand.nif", getPlayerNodes()->primaryWandNode));
		// attachElement(element, getPlayerNodes()->primaryUIAttachNode);
	}

	/**
	 * Attach the UI just below the HMD (head mounted display) direct view. Bound to horizontal but not vertical head movement.
	 */
	void UIManager::attachPresetToHMDBottom(const std::shared_ptr<UIElement>& element) {
		element->setPosition(0, 35, -40);
		attachElement(element, findNode("world_HMD_info.nif", getPlayerNodes()->UprightHmdNode));
	}

	/**
	 * Remove the element and subtree from attached game node.
	 * Safe Release: If <true>, the element will be added to release queue to be released on the next frame update
	 * so finishing access to it on this frame update is still safe (release UI while handling UI event).
	 */
	void UIManager::detachElement(const std::shared_ptr<UIElement>& element, const bool releaseSafe) {
		element->detachFromAttachedNode(releaseSafe);

		// only the root can exists in the manager collection
		if (element->getParent()) {
			return;
		}
		for (auto it = _rootElements.begin(); it != _rootElements.end(); ++it) {
			if (it->get() == element.get()) {
				if (releaseSafe) {
					_releaseSafeList.push_back(*it);
				}
				_MESSAGE("UI Manager root element removed (ReleaseSafe: %d)", releaseSafe);
				_rootElements.erase(it);
				break;
			}
		}
	}

	/**
	 * Dump all the managed UI elements trees in a nice tree format.
	 */
	void UIManager::dumpUITree() const {
		if (_rootElements.empty()) {
			_MESSAGE("--- UI Manager EMPTY ---");
			return;
		}

		for (const auto& element : _rootElements) {
			_MESSAGE("--- UI Manager Root ---");
			dumpUITreeRecursive(element.get(), "");
		}
	}

	void UIManager::dumpUITreeRecursive(UIElement* element, std::string padding) {
		_MESSAGE("%s%s", padding.c_str(), element->toString().c_str());
		const auto container = dynamic_cast<UIContainer*>(element);
		if (!container) {
			return;
		}
		padding += "..";
		for (const auto& child : container->childElements()) {
			dumpUITreeRecursive(child.get(), padding);
		}
	}
}
