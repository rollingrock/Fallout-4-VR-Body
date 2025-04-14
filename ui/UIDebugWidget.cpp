#include "UIDebugWidget.h"

namespace ui {
	void UIDebugWidget::onFrameUpdate(UIModAdapter* adapter) {
		if (!_attachNode) {
			return;
		}

		if (_followInteractionPosition) {
			const auto finger = adapter->getInteractionBonePosition();
			const auto diff = finger - _node->m_worldTransform.pos;
			if (!std::isnan(diff.x) && !std::isnan(diff.y) && !std::isnan(diff.z) && F4VRBody::vec3_len(diff) < 500) {
				_node->m_localTransform.pos += diff;
			}
			else {
				_node->m_localTransform.pos = NiPoint3(0, 0, 0);
			}
		}
	}
}
