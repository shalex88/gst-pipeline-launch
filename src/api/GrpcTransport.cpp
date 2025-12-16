#include "GrpcTransport.h"

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "api/GrpcCallbackHandler.h"
#include "api/IRequestHandler.h"
#include "common/logger/Logger.h"

namespace service::api {
    GrpcTransport::GrpcTransport(IRequestHandler& request_handler) {
        callback_handler_ = std::make_unique<GrpcCallbackHandler>(request_handler);
    }

    GrpcTransport::~GrpcTransport() {
        if (stop().isError()) {
            LOG_ERROR("Failed to stop the gRPC server");
        }
    }

    Result<void> GrpcTransport::start(const std::string& server_address) {
        LOG_DEBUG("Starting server...");
        //TODO: learn how to use health check
        grpc::EnableDefaultHealthCheckService(false);
        //TODO: disable in production
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
        int selected_port = 0;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &selected_port);
        builder.RegisterService(callback_handler_.get());

        // Temporarily suppress STDERR to hide gRPC internal error messages
        const int stderr_backup = dup(STDERR_FILENO);
        const int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDERR_FILENO);
        close(devnull);

        server_ = std::unique_ptr(builder.BuildAndStart());

        // Restore STDERR
        dup2(stderr_backup, STDERR_FILENO);
        close(stderr_backup);

        if (!server_ || selected_port == 0) {
            return Result<void>::error("Failed to open server on: " + server_address);
        }

        is_running_ = true;
        LOG_INFO("Server is listening on: {}", server_address); //TODO: show actual target ip
        return Result<void>::success();
    }

    Result<void> GrpcTransport::stop() {
        if (!is_running_) {
            return Result<void>::success();
        }

        if (server_) {
            LOG_DEBUG("Shutting down server with 30s deadline...");
            server_->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(30));
            server_.reset();
        }

        is_running_ = false;
        LOG_DEBUG("Server stopped");
        return Result<void>::success();
    }

    Result<void> GrpcTransport::runLoop() {
        if (!server_) {
            return Result<void>::error("Server isn't initialized");
        }
        server_->Wait();
        return Result<void>::success();
    }
}