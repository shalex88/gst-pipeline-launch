#ifndef PERIPHERY_MANAGER_NETWORKINTERFACE_H
#define PERIPHERY_MANAGER_NETWORKINTERFACE_H

#include <string>
#include <system_error>

class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;
    virtual std::error_code init() = 0;
    virtual int acceptConnection() = 0;
    virtual std::pair<std::vector<char>, bool> readData(int client_socket) = 0;
    virtual std::error_code sendData(int client_socket, const std::vector<char> data) = 0;
    virtual void closeConnection() = 0;
    virtual int getServerSocket() = 0;
};

#endif //PERIPHERY_MANAGER_NETWORKINTERFACE_H
