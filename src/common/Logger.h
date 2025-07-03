#pragma once

// ReSharper disable CppUnnamedNamespaceInHeaderFile
// ReSharper disable CppClangTidyBugproneMacroParentheses

#include <chrono>
#include <unordered_map>
#include <spdlog/sinks/rotating_file_sink.h>

namespace fs = std::filesystem;

#pragma once

#define MAKE_SOURCE_LOGGER(log_func, log_level)                                                                                                       \
                                                                                                                                                      \
    template <class... Args>                                                                                                                          \
    struct [[maybe_unused]] log_func                                                                                                                  \
    {                                                                                                                                                 \
        log_func() = delete;                                                                                                                          \
                                                                                                                                                      \
        explicit log_func(spdlog::format_string_t<Args...> fmt, Args&&... args, const std::source_location& loc = std::source_location::current())    \
        {                                                                                                                                             \
            spdlog::source_loc sourceLoc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() };                                       \
            internal::_logger->log(sourceLoc, spdlog::level::log_level, fmt, std::forward<Args>(args)...);                                                       \
        }                                                                                                                                             \
    };                                                                                                                                                \
                                                                                                                                                      \
    template <class... Args>                                                                                                                          \
    log_func(spdlog::format_string_t<Args...>, Args&&...) -> log_func<Args...>;

namespace common::logger::internal
{
    /**
     * Global logger instance
     */
    inline std::shared_ptr<spdlog::logger> _logger;

    /**
     * Current log level
     */
    inline int _logLevel = -1;

    /**
     * Holds the last time of a log message per key.
     */
    inline std::unordered_map<std::string_view, std::chrono::steady_clock::time_point> _sampleMessagesTtl;

    /**
     * Same as calling _MESSAGE but only one message log per "time" second, other logs are dropped.
     */
    template <class... Args>
    void sampleImpl(const int time, spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        const fmt::basic_string_view<char> fmtView = fmt;
        const std::string_view key(fmtView.data(), fmtView.size());

        const auto now = std::chrono::steady_clock::now();
        if (_sampleMessagesTtl.contains(key) && now - _sampleMessagesTtl[key] <= std::chrono::milliseconds(time)) {
            return;
        }

        _sampleMessagesTtl[key] = now;
        std::string formatted = fmt::format(fmt, std::forward<Args>(args)...);
        _logger->log(spdlog::level::info, "[SAMPLE] {}", formatted);
    }
}

namespace common::logger
{
    MAKE_SOURCE_LOGGER(trace, trace);
    MAKE_SOURCE_LOGGER(debug, debug);
    MAKE_SOURCE_LOGGER(info, info);
    MAKE_SOURCE_LOGGER(warn, warn);
    MAKE_SOURCE_LOGGER(error, err);
    MAKE_SOURCE_LOGGER(critical, critical);

    /**
     * Same as calling info() but only one message log per "time" in milliseconds, other logs are dropped.
     * Use the message format as a key to identify the log messages that should be sampled.
     */
    template <class... Args>
    void sample(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        internal::sampleImpl(1000, fmt, std::forward<Args>(args)...);
    }

    template <class... Args>
    void sample(const int time, spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        internal::sampleImpl(time, fmt, std::forward<Args>(args)...);
    }

    template <class... Args>
    void infoRaw(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        // TODO:: handle no extra formatting logging
        internal::_logger->log(spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    /**
     * Init logging using a log with the given name put in "My Games" folder.
     */
    inline void init(const std::string_view& logFileName)
    {
        auto path = F4SE::log::log_directory();
        const auto gamepath = REL::Module::IsVR() ? "Fallout4VR/F4SE" : "Fallout4/F4SE";
        if (!path.value().generic_string().ends_with(gamepath)) {
            // handle bug where game directory is missing
            path = path.value().parent_path().append(gamepath);
        }

        // Use rolling log files (5 files, max 10mb each)
        *path /= fmt::format("{}.log"sv, logFileName);
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path->string(), 1024 * 1024 * 10, 5, true);

        internal::_logger = std::make_shared<spdlog::logger>("GLOBAL"s, std::move(sink));

        constexpr auto level = spdlog::level::info;
        internal::_logger->set_level(level);
        internal::_logger->flush_on(level);

        spdlog::set_default_logger(internal::_logger);

        // see: https://github.com/gabime/spdlog/wiki/Custom-formatting
        spdlog::set_pattern("%H:%M:%S.%e %L: %v"s);
    }

    /**
     * Update the global logger log level based on the config setting.
     */
    inline void setLogLevel(int logLevel)
    {
        if (internal::_logLevel == logLevel) {
            return;
        }

        info("Set log level = {}", logLevel);
        internal::_logLevel = logLevel;
        const auto levelEnum = static_cast<spdlog::level::level_enum>(logLevel);
        internal::_logger->set_level(levelEnum);
        internal::_logger->flush_on(levelEnum);
    }
}

#undef MAKE_SOURCE_LOGGER
