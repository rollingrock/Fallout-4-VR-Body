#pragma once

#include "UIWidget.h"

namespace ui {
	class UIDebugWidget : public UIWidget {
	public:
		UIDebugWidget()
			: UIWidget(getDebugSphereNifName()) {}

		virtual void onFrameUpdate(UIModAdapter* adapter) override;

		[[nodiscard]] bool isFollowInteractionPosition() const { return _followInteractionPosition; }

		void setFollowInteractionPosition(const bool followInteractionPosition) {
			_followInteractionPosition = followInteractionPosition;
		}

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		bool _followInteractionPosition = false;
	};
}
