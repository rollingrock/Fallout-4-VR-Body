#pragma once

#include "UIWidget.h"

namespace vrui
{
    class UIDebugWidget : public UIWidget
    {
    public:
        explicit UIDebugWidget(const bool followInteractPos = false) :
            UIWidget(getDebugSphereNifName())
        {
            _followInteractionPosition = followInteractPos;
        }

        virtual void onFrameUpdate(UIFrameUpdateContext* adapter) override;

        bool isFollowInteractionPosition() const { return _followInteractionPosition; }

        void setFollowInteractionPosition(const bool followInteractionPosition)
        {
            _followInteractionPosition = followInteractionPosition;
        }

        virtual std::string toString() const override;

    protected:
        bool _followInteractionPosition = false;
    };
}
