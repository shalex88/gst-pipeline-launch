#ifndef PERIPHERY_MANAGER_TCPNETWORKMANAGER_H
#define PERIPHERY_MANAGER_TCPNETWORKMANAGER_H

#include <vector>
#include "NetworkInterface.h"

class TcpNetworkManager : public NetworkInterface {
public:
    explicit TcpNetworkManager(int port);
    ~TcpNetworkManager() override = default;
    std::error_code init() override;
    int acceptConnection() override;
    std::pair<std::vector<char>, bool> readData(int client_socket) override;
    std::error_code sendData(int client_socket, const std::vector<char> data) override;
    void closeConnection() override;
    int getServerSocket() override;

private:
    int server_socket_{-1};
    int port_{-1};
};

#endif //PERIPHERY_MANAGER_TCPNETWORKMANAGER_H
