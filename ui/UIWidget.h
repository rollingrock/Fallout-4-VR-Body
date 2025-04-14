#pragma once

#include "UIElement.h"
#include "UIUtils.h"

namespace ui {
	class UIWidget : public UIElement {
	public:
		explicit UIWidget(const std::string& nifPath)
			: UIWidget(getClonedNiNodeForNifFile(nifPath)) {}

		explicit UIWidget(NiNode* node)
			: _node(node) {}

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		[[nodiscard]] virtual bool isPressable() const { return false; }
		virtual void attachToNode(NiNode* attachNode) override;
		virtual void detachFromAttachedNode(bool releaseSafe) override;
		virtual void onFrameUpdate(UIModAdapter* adapter) override;
		virtual NiTransform calculateTransform() override;
		virtual void onPressEventFired(UIElement* element, UIModAdapter* adapter) override;
		void handlePressEvent(UIModAdapter* adapter);

		// UI node to render
		NiPointer<NiNode> _node;

		// Press handling
		bool _pressEventFired = false;
		float _pressYOffset = 0;
	};
}
