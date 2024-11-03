#include <memory>
#include "Logger/Logger.h"
#include "Gstreamer/Gstreamer.h"

int main([[maybe_unused]] const int argc, [[maybe_unused]] char* argv[]) {
    SET_LOG_LEVEL(LoggerInterface::LogLevel::Trace);

    auto gstreamer = std::make_shared<Gstreamer>("../resources/pipeline.json");

    std::thread user_input_thread([gstreamer]()  {
        LOG_INFO("Enter 'e' to enable optional element, 'd' to disable optional element, and 'q' to quit:");
        char command;
        while (true) {
            std::cin >> command;
            if (command == 'e') {
                gstreamer->enable_optional_pipeline_elements();
            } else if (command == 'd') {
                gstreamer->disable_optional_pipeline_elements();
            } else if (command == 'q') {
                gstreamer->stop();
                break;
            }
        }
    });

    gstreamer->play();

    user_input_thread.join();

    return EXIT_SUCCESS;
}
