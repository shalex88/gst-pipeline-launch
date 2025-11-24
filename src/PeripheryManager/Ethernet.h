#ifndef PERIPHERY_MANAGER_ETHERNET_H
#define PERIPHERY_MANAGER_ETHERNET_H

#include <string>
#include <arpa/inet.h>
#include "PeripheryManager/HwInterface.h"

class Ethernet : public HwInterface {
public:
    Ethernet(const std::string& server_ip, int server_port);
    ~Ethernet() override = default;
    std::vector<uint8_t> read() override;
    size_t write(const std::vector<uint8_t>& tx_data) override;
    bool init() override;
    bool deinit() override;

private:
    int client_socket_{-1};
    sockaddr_in server_address_{};
};

#endif //PERIPHERY_MANAGER_ETHERNET_H
