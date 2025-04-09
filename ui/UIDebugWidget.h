#pragma once

#include "UIWidget.h"

namespace ui {

	class UIDebugWidget : public UIWidget {
	public:
		UIDebugWidget()
			: UIWidget(getDebugSphereNif()) {
		}

		void onFrameUpdate(IUIModAdapter* adapter) override;
	};
}
