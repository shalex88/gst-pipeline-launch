#pragma once

#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>

#include "LoggerInterface.h"

class SpdLogAdapter final : public LoggerInterface {
public:
    SpdLogAdapter() : SpdLogAdapter(APP_NAME) {}

    explicit SpdLogAdapter(const std::string& logger_name) {
        auto stdout_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(std::cout, false);
        logger_ = std::make_shared<spdlog::logger>(logger_name, stdout_sink);
        set_default_logger(logger_);
        logger_->set_level(spdlog::level::info);
    }

    ~SpdLogAdapter() override {
        spdlog::drop_all();
    }

    void setLogLevel(const LogLevel level) override {
        logger_->set_level(toSpdLogLevel(level));
    }

    void setLogLevel(const std::string& level) override {
        if (level == "trace") {
            setLogLevel(LogLevel::Trace);
        } else if (level == "debug") {
            setLogLevel(LogLevel::Debug);
        } else if (level == "info") {
            setLogLevel(LogLevel::Info);
        } else if (level == "warn") {
            setLogLevel(LogLevel::Warn);
        } else if (level == "error") {
            setLogLevel(LogLevel::Error);
        } else if (level == "critical") {
            setLogLevel(LogLevel::Critical);
        } else {
            logger_->error("Invalid log severity: {}, defaulting to info", level);
            setLogLevel(LogLevel::Info);
        }
    }

    void logImpl(const LogLevel level, const std::string &msg) override {
        logger_->log(toSpdLogLevel(level), msg);
    }

private:
    std::shared_ptr<spdlog::logger> logger_;

    static spdlog::level::level_enum toSpdLogLevel(const LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return spdlog::level::trace;
            case LogLevel::Debug:
                return spdlog::level::debug;
            case LogLevel::Info:
                return spdlog::level::info;
            case LogLevel::Warn:
                return spdlog::level::warn;
            case LogLevel::Error:
                return spdlog::level::err;
            case LogLevel::Critical:
                return spdlog::level::critical;
            default:
                return spdlog::level::info;
        }
    }
};