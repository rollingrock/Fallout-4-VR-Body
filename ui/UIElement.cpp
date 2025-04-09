#include "UIElement.h"

namespace ui {

	/// <summary>
	/// calculate the transform of the element with respect to all parents.
	/// </summary>
	NiTransform UIElement::calculateTransform()	{
		if (!_parent) {
			return _transform;
		}
		
		auto calTransform = _transform;
		auto parentTransform = _parent->calculateTransform();
		calTransform.pos += parentTransform.pos;
		// TODO: add rotation handling
		return calTransform;
	}
	
	/// <summary>
	/// Calculate if the element should be visible with respect to all parents.
	/// </summary>
	bool UIElement::calculateVisibility() {
		return _visible && (_parent ? _parent->calculateVisibility() : true);
	}
}