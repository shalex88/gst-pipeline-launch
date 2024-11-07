#include "SignalHandler.h"
#include "Logger/Logger.h"

void SignalHandler::setupSignalHandling() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
}

void SignalHandler::resetSignalHandling() {
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGSEGV, SIG_DFL);
    std::signal(SIGABRT, SIG_DFL);
}

void SignalHandler::signalHandler(int signal) {
    LOG_DEBUG("Signal {} received", signal);
    SignalHandler::resetSignalHandling();
    App::shutdown();
}
