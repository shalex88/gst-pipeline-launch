#ifndef PERIPHERY_MANAGER_INPUTINTERFACE_H
#define PERIPHERY_MANAGER_INPUTINTERFACE_H

#include <string>
#include <utility>

class InputInterface : public std::enable_shared_from_this<InputInterface> {
public:
    struct Requester {
        std::shared_ptr<InputInterface> source;
        int source_id;
        Requester(std::shared_ptr<InputInterface> input_interface, int id) : source(std::move(input_interface)), source_id(id) {}
    };
    virtual void sendResponse(std::shared_ptr<InputInterface::Requester> requester, const std::string& response) = 0;
    virtual ~InputInterface() = default;
};

#endif //PERIPHERY_MANAGER_INPUTINTERFACE_H
