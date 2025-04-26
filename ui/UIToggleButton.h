#pragma once

#include "UIWidget.h"

namespace ui {
	class UIToggleButton : public UIWidget {
	public:
		explicit UIToggleButton(const std::string& nifPath)
			: UIToggleButton(getClonedNiNodeForNifFile(nifPath)) {}

		explicit UIToggleButton(NiNode* node)
			: UIWidget(node), _toggleFrameNode(getClonedNiNodeForNifFile(getToggleButtonFrameNifName())) {
			// TODO: replace with proper calculation of node size
			_size = getButtonDefaultSize();
		}

		// is the button is currently toggled ON or OFF
		[[nodiscard]] bool isToggleOn() const { return _isToggleOn; }
		void setToggleState(const bool isToggleOn) { _isToggleOn = isToggleOn; }

		// is a user is allowed to un-toggle the button (useful for toggle group)
		[[nodiscard]] bool isUnToggleAllowed() const { return _isUnToggleAllowed; }
		void setUnToggleAllowed(const bool allowUnToggle) { _isUnToggleAllowed = allowUnToggle; }

		void setOnToggleHandler(std::function<void(UIWidget*, bool)> handler) {
			_onToggleEventHandler = std::move(handler);
		}

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		virtual void attachToNode(NiNode* node) override;
		virtual void detachFromAttachedNode(bool releaseSafe) override;
		[[nodiscard]] virtual bool isPressable() const override;
		virtual void onFrameUpdate(UIFrameUpdateContext* adapter) override;
		virtual void onPressEventFired(UIElement* element, UIFrameUpdateContext* context) override;

		std::function<void(UIToggleButton*, bool)> _onToggleEventHandler;
		bool _isToggleOn = false;
		bool _isUnToggleAllowed = true;

		// the UI of the white frame around the button
		NiPointer<NiNode> _toggleFrameNode;
	};
}
