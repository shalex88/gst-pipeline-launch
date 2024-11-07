#include "MessageServer.h"
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include "Logger/Logger.h"

MessageServer::MessageServer(std::shared_ptr<CommandDispatcher> command_dispatcher, std::shared_ptr<NetworkInterface> network_manager) :
    command_dispatcher_(std::move(command_dispatcher)), network_manager_(std::move(network_manager)) {
}

MessageServer::~MessageServer() {
    deinit();
}

bool MessageServer::init() {
    keep_running_ = true;
    server_thread_ = std::thread(&MessageServer::runServer, this);

    return true;
}

bool MessageServer::deinit() {
    keep_running_ = false;

    stopAllClientThreads();

    network_manager_->closeConnection();

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    LOG_INFO("[Message Server] Stopped");

    return true;
}

void MessageServer::runServer() {
    auto ec = network_manager_->init();
    if (ec) {
        LOG_ERROR("[Message Server] {}", ec.message());
    }

    LOG_INFO("[Message Server] Started");

    while (keep_running_) {
        if (const int client_socket = network_manager_->acceptConnection(); client_socket >= 0) {
            LOG_TRACE("[Message Server] Client {} connected", client_socket);
            std::lock_guard<std::mutex> lock(client_threads_mutex_);
            client_threads_.emplace_back(&MessageServer::handleClient, this, client_socket);
        }
    }
}

void MessageServer::handleClient(int client_socket) {
    while (keep_running_) {
        auto [data, disconnect] = network_manager_->readData(client_socket);

        if (disconnect) {
            break;
        }

        if (!data.empty()) {
            parseMessage(client_socket, data);
        }
    }

    close(client_socket);
    LOG_TRACE("[Message Server] Client {} disconnected", client_socket);
}

bool MessageServer::parseMessage(const int client, const std::vector<char>& buffer) {
    LOG_TRACE("{}", printMessage(client, buffer));

    auto requester = std::make_shared<InputInterface::Requester>(shared_from_this(), client);
    command_dispatcher_->dispatchCommand(requester, std::string(buffer.begin(), buffer.end()));

    return true;
}

std::string MessageServer::printMessage(const int client, const std::vector<char>& buffer) const {
    std::ostringstream os;
    os << "[Message Server] Received from client " << client <<" (" << buffer.size() << " bytes): ";
    os << std::string(buffer.begin(), buffer.end()) << " [";

    for (const auto& c : buffer) {
        os << static_cast<int>(static_cast<unsigned char>(c)) << " ";
    }

    os << "]";
    return os.str();
}

void MessageServer::sendResponse(std::shared_ptr<InputInterface::Requester> requester, const std::string& response) {
    std::vector<char> data(response.begin(), response.end());
    auto ec = network_manager_->sendData(requester->source_id, data);
    if (ec) {
        LOG_ERROR("[Message Server] {}", ec.message());
    }
}

void MessageServer::stopAllClientThreads() {
    if (!client_threads_.empty()) {
        for (auto& thread: client_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        client_threads_.clear();
    }
}
