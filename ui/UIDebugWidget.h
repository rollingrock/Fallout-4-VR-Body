#pragma once

#include "UIWidget.h"

namespace ui {
	class UIDebugWidget : public UIWidget {
	public:
		explicit UIDebugWidget(const bool followInteractPos = false)
			: UIWidget(getDebugSphereNifName()) {
			_followInteractionPosition = followInteractPos;
		}

		virtual void onFrameUpdate(UIFrameUpdateContext* adapter) override;

		[[nodiscard]] bool isFollowInteractionPosition() const { return _followInteractionPosition; }

		void setFollowInteractionPosition(const bool followInteractionPosition) {
			_followInteractionPosition = followInteractionPosition;
		}

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		bool _followInteractionPosition = false;
	};
}
