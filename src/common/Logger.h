#pragma once

#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "CommonUtils.h"

namespace fs = std::filesystem;

namespace common {
	class Log {
	public:
		/**
		 * Init logging using a log with the given name put in "My Games" folder.
		 */
		static void init(const std::string& logFileName) {
			const auto path = getRelativePathInDocuments(R"(\My Games\Fallout4VR\F4SE\)" + logFileName);
			rollLogFiles(path, 6);

			IDebugLog::Open(path.c_str());
			IDebugLog::SetPrintLevel(IDebugLog::kLevel_Message);
			IDebugLog::SetLogLevel(IDebugLog::kLevel_Message);
		}

		/**
		 * Update the global logger log level based on the config setting.
		 */
		static void setLogLevel(int logLevel) {
			if (_logLevel == logLevel)
				return;

			info("Set log level = %d", logLevel);
			_logLevel = logLevel;
			const auto level = static_cast<IDebugLog::LogLevel>(logLevel);
			IDebugLog::SetPrintLevel(level);
			IDebugLog::SetLogLevel(level);
		}

		static void verbose(const char* fmt, ...) {
			if (_logLevel < IDebugLog::kLevel_VerboseMessage)
				return;

			va_list args;
			va_start(args, fmt);
#ifdef _DEBUG // don't do extra work in release
			const auto msg = getTimeStringForLog() + " VERB " + fmt;
			fmt = msg.c_str();
#endif
			IDebugLog::Log(IDebugLog::kLevel_VerboseMessage, fmt, args);
			va_end(args);
		}

		static void debug(const char* fmt, ...) {
			if (_logLevel < IDebugLog::kLevel_DebugMessage)
				return;

			va_list args;
			va_start(args, fmt);
#ifdef _DEBUG // don't do extra work in release
			const auto msg = getTimeStringForLog() + " DEBUG " + fmt;
			fmt = msg.c_str();
#endif
			IDebugLog::Log(IDebugLog::kLevel_DebugMessage, fmt, args);
			va_end(args);
		}

		static void info(const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
#ifdef _DEBUG // don't do extra work in release
			const auto msg = getTimeStringForLog() + " INFO " + fmt;
			fmt = msg.c_str();
#endif
			IDebugLog::Log(IDebugLog::kLevel_Message, fmt, args);
			va_end(args);
		}

		static void warn(const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
#ifdef _DEBUG // don't do extra work in release
			const auto msg = getTimeStringForLog() + " WARN " + fmt;
			fmt = msg.c_str();
#endif
			IDebugLog::Log(IDebugLog::kLevel_Warning, fmt, args);
			va_end(args);
		}

		static void error(const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
			const auto msg = getTimeStringForLog() + " ERROR " + fmt;
			fmt = msg.c_str();
			IDebugLog::Log(IDebugLog::kLevel_Error, fmt, args);
			va_end(args);
		}

		static void fatal(const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
			const auto msg = getTimeStringForLog() + " FATAL " + fmt;
			fmt = msg.c_str();
			IDebugLog::Log(IDebugLog::kLevel_FatalError, fmt, args);
			va_end(args);
		}

		/**
		 * Same as calling info() but only one message log per "time" in milliseconds, other logs are dropped.
		 * Use static key to identify the log messages that should be sampled.
		 */
		static void sample(const std::string& key, const int time, const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
			sampleImpl(key, time, fmt, args);
			va_end(args);
		}

		/**
		 * Same as calling info() but only one message log per second, other logs are dropped.
		 * Use static key to identify the log messages that should be sampled.
		 */
		static void sample(const std::string& key, const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
			sampleImpl(key, 1000, fmt, args);
			va_end(args);
		}

	private:
		/**
		 * Get a simple string of the current time in HH:MM:SS.ms format.
		 */
		static std::string getTimeStringForLog() {
			const auto now = std::chrono::system_clock::now();
			const auto nowC = std::chrono::system_clock::to_time_t(now);
			std::tm localTime;
			localtime_s(&localTime, &nowC); // NOLINT(cert-err33-c)
			const auto ms = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
			std::ostringstream oss;
			oss << std::put_time(&localTime, "%H:%M:%S")
				<< '.' << std::setfill('0') << std::setw(3) << ms;
			return oss.str();
		}

		/**
		 * Same as calling _MESSAGE but only one message log per "time" second, other logs are dropped.
		 */
		static void sampleImpl(const std::string& key, const int time, const char* fmt, const va_list args) {
			const auto now = std::chrono::steady_clock::now();
			if (_sampleMessagesTtl.contains(key) && now - _sampleMessagesTtl[key] <= std::chrono::milliseconds(time)) {
				return;
			}
			_sampleMessagesTtl[key] = now;
			const auto msg = getTimeStringForLog() + " SMPL " + fmt;
			IDebugLog::Log(IDebugLog::kLevel_Message, msg.c_str(), args);
		}

		/**
		 * Roll the logfiles to have up to <max> old log files available.
		 * The latest will be "name.log", previous "name_1.log", until "name_<max>.log"
		 */
		static void rollLogFiles(const std::string& logPathStr, const int max) {
			if (max < 2)
				return;

			const fs::path logPath(logPathStr);
			const auto parent = logPath.parent_path();
			const auto stem = logPath.stem().string(); // "name"
			const auto ext = logPath.extension().string(); // ".log"

			// Delete the oldest one if it exists (name_6.log)
			const fs::path oldest = parent / (stem + "_" + std::to_string(max) + ext);
			if (fs::exists(oldest)) {
				fs::remove(oldest);
			}

			// Roll logs: name_5.log -> name_6.log, ..., name_1.log -> name_2.log
			for (int i = max - 1; i >= 1; --i) {
				fs::path src = parent / std::format("{}_{}{}", stem, i, ext);
				fs::path dst = parent / std::format("{}_{}{}", stem, i + 1, ext);
				if (fs::exists(src)) {
					fs::rename(src, dst);
				}
			}

			// name.log -> name_1.log
			if (fs::exists(logPath)) {
				fs::rename(logPath, parent / std::format("{}_1{}", stem, ext));
			}
		}

		/**
		 * Current log level
		 */
		inline static int _logLevel = -1;

		/**
		 * Holds the last time of a log message per key.
		 */
		inline static std::unordered_map<std::string, std::chrono::steady_clock::time_point> _sampleMessagesTtl;
	};
}
