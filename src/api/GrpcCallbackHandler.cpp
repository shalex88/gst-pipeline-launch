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

    grpc::ServerUnaryReactor* GrpcCallbackHandler::SetVideoCapabilityState(
        grpc::CallbackServerContext* context,
        const video::v1::SetVideoCapabilityStateRequest* request,
        google::protobuf::Empty* response) {
        return handleGrpcRequest(context, request, response,
            [this](const video::v1::SetVideoCapabilityStateRequest* req, google::protobuf::Empty* resp) {
                return request_handler_.setVideoCapability(req->capability(), req->enable());
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::GetVideoCapabilities(
        grpc::CallbackServerContext* context,
        const google::protobuf::Empty* request,
        video::v1::GetVideoCapabilitiesResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const google::protobuf::Empty* req, video::v1::GetVideoCapabilitiesResponse* resp) {
                auto operation = request_handler_.getVideoCapabilities();
                if (operation.isError()) {
                    return Result<void>::error(operation.error());
                }

                for (const auto& capability : operation.value()) {
                    resp->add_capabilities(capability);
                }

                return Result<void>::success();
            });
    }

    grpc::ServerUnaryReactor* GrpcCallbackHandler::GetVideoCapabilityState(
        grpc::CallbackServerContext* context,
        const video::v1::GetVideoCapabilityStateRequest* request,
        video::v1::GetVideoCapabilityStateResponse* response) {
        return handleGrpcRequest(context, request, response,
            [this](const video::v1::GetVideoCapabilityStateRequest* req, video::v1::GetVideoCapabilityStateResponse* resp) {
                auto operation = request_handler_.getVideoCapabilityState(req->capability());
                if (operation.isError()) {
                    return Result<void>::error(operation.error());
                }

                resp->set_enable(operation.value());
                return Result<void>::success();
            });
    }
}
