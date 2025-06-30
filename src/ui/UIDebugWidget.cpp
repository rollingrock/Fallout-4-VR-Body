#include "UIDebugWidget.h"

#include <format>

#include "common/MatrixUtils.h"

using namespace common;

namespace vrui {
	std::string UIDebugWidget::toString() const {
		return std::format("UIDebugWidget: {}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
			_visible ? "V" : "H",
			isPressable() ? "F" : ".",
			_transform.translate.x,
			_transform.translate.y,
			_transform.translate.z,
			_size.width,
			_size.height
		);
	}

	void UIDebugWidget::onFrameUpdate(UIFrameUpdateContext* adapter) {
		UIWidget::onFrameUpdate(adapter);
		if (!_attachNode) {
			return;
		}

		if (_followInteractionPosition) {
			const auto finger = adapter->getInteractionBoneWorldPosition();
			const auto diff = finger - _node->world.translate;
			if (!std::isnan(diff.x) && !std::isnan(diff.y) && !std::isnan(diff.z) && vec3Len(diff) < 500) {
				_node->local.translate += diff;
			} else {
				_node->local.translate = RE::NiPoint3(0, 0, 0);
			}
		}
	}
}
