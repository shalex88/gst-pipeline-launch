#ifndef PERIPHERY_MANAGER_LOGGERINTERFACE_H
#define PERIPHERY_MANAGER_LOGGERINTERFACE_H

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
    void log(LogLevel level, const std::string &format, Args &&... args) {
        logImpl(level, fmt::format(format, std::forward<Args>(args)...));
    }

    virtual void setLogLevel(LogLevel level) = 0;

protected:
    virtual void logImpl(LogLevel level, const std::string &msg) = 0;
};

#endif //PERIPHERY_MANAGER_LOGGERINTERFACE_H