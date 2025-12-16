#pragma once

#include <string>

#include <fmt/format.h>

class LoggerInterface {
public:
    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

    virtual ~LoggerInterface() = default;

    template<typename... Args>
    void log(const LogLevel level, const std::string& format_str, Args&&... args) {
        const std::string formatted_str = fmt::format(fmt::runtime(format_str), std::forward<Args>(args)...);
        logImpl(level, formatted_str);
    }

    virtual void setLogLevel(LogLevel level) = 0;
    virtual void setLogLevel(const std::string& level) = 0;

protected:
    virtual void logImpl(LogLevel level, const std::string &msg) = 0;
};