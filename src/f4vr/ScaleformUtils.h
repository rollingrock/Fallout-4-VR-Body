#pragma once

#include <functional>
#include <optional>

namespace f4vr
{
    enum class ScaleformListOp : std::uint8_t
    {
        MoveUp = 0,
        MoveDown,
        Select,
    };

    bool getScaleformBool(const RE::Scaleform::GFx::Movie* root, const char* path);
    std::optional<int> getScaleformInt(const RE::Scaleform::GFx::Movie* root, const char* path);
    bool isElementVisible(const RE::Scaleform::GFx::Movie* root, const std::string& path);
    bool doOperationOnScaleformList(RE::Scaleform::GFx::Movie* root, const char* listPath, ScaleformListOp op);
    bool doOperationOnScaleformMessageHolderList(RE::Scaleform::GFx::Movie* root, const char* messageHolderPath, ScaleformListOp op);

    void invokeScaleformProcessUserEvent(RE::Scaleform::GFx::Movie* root, const std::string& path, const char* eventName);
    void invokeScaleformDispatchEvent(RE::Scaleform::GFx::Movie* root, const std::string& path, const char* eventName);

    bool findAndWorkOnScaleformElement(RE::Scaleform::GFx::Value* elm, const std::string& name, const std::function<void(RE::Scaleform::GFx::Value&)>& doWork);
}
