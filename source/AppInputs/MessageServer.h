#ifndef PERIPHERY_MANAGER_MESSAGESERVER_H
#define PERIPHERY_MANAGER_MESSAGESERVER_H

#include "TasksManager/CommandDispatcher.h"
#include "Network/NetworkInterface.h"
#include "InputInterface.h"
#include <atomic>
#include <iostream>
#include <list>
#include <thread>
#include <vector>

class MessageServer : public InputInterface {
public:
    MessageServer(std::shared_ptr<CommandDispatcher> command_dispatcher, std::shared_ptr<NetworkInterface> network_manager);
    ~MessageServer() override;
    bool init();
    bool deinit();
    void sendResponse(std::shared_ptr<InputInterface::Requester> requester, const std::string& response) override;

private:
    void runServer();
    bool parseMessage(const int client, const std::vector<char>& buffer);
    std::string printMessage(const int client, const std::vector<char>& buffer) const;
    void handleClient(int client_socket);
    void stopAllClientThreads();
    std::shared_ptr<CommandDispatcher> command_dispatcher_;
    std::shared_ptr<NetworkInterface> network_manager_;
    std::atomic<bool> keep_running_{false};
    std::list<std::thread> client_threads_;
    std::mutex client_threads_mutex_;
    std::thread server_thread_;
};

#endif //PERIPHERY_MANAGER_MESSAGESERVER_H
