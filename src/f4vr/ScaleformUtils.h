#pragma once

#include <functional>
#include <optional>
#include <f4se/ScaleformMovie.h>
#include <f4se/ScaleformValue.h>

namespace f4vr
{
    enum class ScaleformListOp : UInt8
    {
        MoveUp = 0,
        MoveDown,
        Select,
    };

    bool getScaleformBool(const GFxMovieRoot* root, const char* path);
    std::optional<int> getScaleformInt(const GFxMovieRoot* root, const char* path);
    bool isElementVisible(const GFxMovieRoot* root, const std::string& path);
    bool doOperationOnScaleformList(GFxMovieRoot* root, const char* listPath, ScaleformListOp op);
    bool doOperationOnScaleformMessageHolderList(GFxMovieRoot* root, const char* messageHolderPath, ScaleformListOp op);

    void invokeScaleformProcessUserEvent(GFxMovieRoot* root, const std::string& path, const char* eventName);
    void invokeScaleformDispatchEvent(GFxMovieRoot* root, const std::string& path, const char* eventName);

    bool findAndWorkOnScaleformElement(GFxValue* elm, const std::string& name, const std::function<void(GFxValue &)>& doWork);
}
