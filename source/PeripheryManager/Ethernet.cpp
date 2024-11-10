#include "Ethernet.h"
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Logger/Logger.h"

const int MaxBufferSize {1024};

Ethernet::Ethernet(const std::string &server_ip, int server_port) {
    LOG_DEBUG("Creating a TCP client");
    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket_ != -1) {
        server_address_.sin_family = AF_INET;
        server_address_.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip.c_str(), &server_address_.sin_addr);
    } else {
        LOG_ERROR("Error creating TCP socket");
        throw std::runtime_error("Error creating TCP socket");
    }
}

std::vector<uint8_t> Ethernet::read() {
    std::vector<uint8_t> buffer(MaxBufferSize);
    const ssize_t bytes_received = recv(client_socket_, buffer.data(), buffer.size(), 0);
    if (bytes_received == -1) {
        return {};
    }

    buffer.resize(bytes_received);
    return buffer;
}

size_t Ethernet::write(const std::vector<uint8_t> &tx_data) {
    if (send(client_socket_, tx_data.data(), tx_data.size(), 0) != -1) {
        return tx_data.size();
    }

    return 0;
}

bool Ethernet::init() {
    LOG_DEBUG("Connecting to server...");

    return connect(client_socket_, reinterpret_cast<struct sockaddr*>(&server_address_), sizeof(server_address_)) != -1;
}

bool Ethernet::deinit() {
    LOG_DEBUG("Disconnecting from server");

    return close(client_socket_) != -1;
}