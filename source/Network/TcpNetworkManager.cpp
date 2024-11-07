#include "TcpNetworkManager.h"
#include "Logger/Logger.h"
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <thread>

const int MaxBufferSize {1024};
const int MaxClientsNum {5};

TcpNetworkManager::TcpNetworkManager(int port) :
        port_(port) {}

std::error_code TcpNetworkManager::init() {
    sockaddr_in server_addr{};
    int opt = 1;

    // Creating socket file descriptor
    if (server_socket_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); server_socket_ <= 0) {
        return {errno, std::generic_category()};
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
        close(server_socket_);
        return {errno, std::generic_category()};
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(server_socket_);
        return {errno, std::generic_category()};
    }

    listen(server_socket_, MaxClientsNum);

    return {};
}

int TcpNetworkManager::acceptConnection() {
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    const int client_socket = accept4(server_socket_, reinterpret_cast<struct sockaddr*>(&client_addr),
            &client_addr_len, SOCK_CLOEXEC | O_NONBLOCK);
    if (client_socket < 0) {
        // Non-blocking mode or an actual error
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            // No connections are pending, so do something else or just yield the CPU
            std::this_thread::yield(); // This is one way to avoid busy waiting
        } else {
            return -1;
        }
    } else {
        return client_socket;
    }

    return -1;
}

std::pair<std::vector<char>, bool> TcpNetworkManager::readData(int client_socket) {
    fd_set read_fds;
    struct timeval tv {};
    std::vector<char> buffer(MaxBufferSize);
    bool disconnect{false};

    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);

    // Set timeout to 1 second. Adjust as needed for responsiveness vs. resource usage
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (select(client_socket + 1, &read_fds, nullptr, nullptr, &tv) >= 0 ) {
        ssize_t bytes_read = ::read(client_socket, buffer.data(), buffer.size() - 1);
        if (bytes_read > 0) {
            buffer.resize(bytes_read);
        } else if (bytes_read == 0) {
            disconnect = true;
        } else {
            buffer.clear();
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                LOG_ERROR("[Message Server] Reading failed");
            }
        }
    } else {
        LOG_ERROR("[Message Server] Listening to socket failed");
    }

    return {buffer, disconnect};
}

std::error_code TcpNetworkManager::sendData(int client_socket, const std::vector<char> data) {
    if (send(client_socket, data.data(), data.size(), 0) < 0) {
        return {errno, std::generic_category()};
    }

    return {};
}

void TcpNetworkManager::closeConnection() {
    if (server_socket_ != -1) {
        close(server_socket_);
        server_socket_ = -1;
    }
}

int TcpNetworkManager::getServerSocket() {
    return server_socket_;
}
