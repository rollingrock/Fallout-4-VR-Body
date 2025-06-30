#pragma once

#include "UIElement.h"
#include "UIUtils.h"

namespace vrui {
	class UIWidget : public UIElement {
	public:
		explicit UIWidget(const std::string& nifPath, const float scale = 1)
			: UIWidget(getClonedNiNodeForNifFile(nifPath)) {
			setScale(scale);
		}

		explicit UIWidget(NiNode* node)
			: _node(node) {}

		virtual std::string toString() const override;

	protected:
		virtual bool isPressable() const { return false; }
		virtual void attachToNode(NiNode* attachNode) override;
		virtual void detachFromAttachedNode(bool releaseSafe) override;
		virtual void onFrameUpdate(UIFrameUpdateContext* adapter) override;
		virtual RE::NiTransform calculateTransform() const override;
		virtual void onPressEventFired(UIElement* element, UIFrameUpdateContext* context) override;
		void handlePressEvent(UIFrameUpdateContext* context);
		void updatePressableCloseToInteraction(UIFrameUpdateContext* context, float distance, float yOnlyDistance);

		// UI node to render
		NiPointer<NiNode> _node;

		// Press handling
		bool _pressEventFired = false;
		float _pressYOffset = 0;

		// To handle pressable close to interaction bone margin to fix twitching because of change from true to false
		bool _wasPressableCloseToInteraction = false;
	};
}
