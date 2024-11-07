#ifndef PERIPHERY_MANAGER_SIGNALHANDLER_H
#define PERIPHERY_MANAGER_SIGNALHANDLER_H

#include <csignal>
#include "App/App.h"

class SignalHandler {
public:
    static void setupSignalHandling();
    static void resetSignalHandling();

private:
    static void signalHandler(int);
};

#endif //PERIPHERY_MANAGER_SIGNALHANDLER_H
