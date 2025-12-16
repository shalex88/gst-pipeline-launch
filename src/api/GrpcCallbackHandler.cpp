#include "GrpcCallbackHandler.h"

#include <future>
#include <grpcpp/grpcpp.h>

#include "api/IRequestHandler.h"
#include "common/logger/Logger.h"

namespace service::api {
    GrpcCallbackHandler::GrpcCallbackHandler(IRequestHandler& request_handler)
        : request_handler_(request_handler) {
    }

    template<typename RequestType, typename ResponseType, typename ProcessFunc>
    grpc::ServerUnaryReactor* handleGrpcRequest(
        grpc::CallbackServerContext* context,
        const RequestType* request,
        ResponseType* response,
        ProcessFunc process_function) {
        const auto reactor = context->DefaultReactor();
        const auto deadline = grpc::Timespec2Timepoint(context->raw_deadline());

        // Calculate the remaining time before the deadline
        const auto now = std::chrono::system_clock::now();
        const auto remaining_time = deadline > now ? deadline - now : std::chrono::seconds(0);

        // Launch the processing task asynchronously
        std::future<grpc::Status> future = std::async(std::launch::async, [request, response, process_function] {
            if (auto result = process_function(request, response); result.isError()) {
                return grpc::Status(grpc::StatusCode::INTERNAL, result.error());
            }
            return grpc::Status::OK;
        });

        //TODO: handle task execution abort if deadline exceeds, think of a way to cancel and undo the task

        // Wait for the processing to complete or timeout
        if (future.wait_for(remaining_time) == std::future_status::timeout) {
            LOG_ERROR("Request exceeded deadline during processing.");
            reactor->Finish(grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED, "Processing exceeded deadline"));
        } else {
            reactor->Finish(future.get());
        }

        return reactor;
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::EnableOptionalElement(
        grpc::CallbackServerContext* context,
        const video::EnableOptionalElementRequest* request,
        video::EnableOptionalElementResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const video::EnableOptionalElementRequest* req, video::EnableOptionalElementResponse* resp) {
                return request_handler_.enableOptionalElement(req->element());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::DisableOptionalElement(
        grpc::CallbackServerContext* context,
        const video::DisableOptionalElementRequest* request,
        video::DisableOptionalElementResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const video::DisableOptionalElementRequest* req, video::DisableOptionalElementResponse* resp) {
                return request_handler_.disableOptionalElement(req->element());
            });
    }
}
