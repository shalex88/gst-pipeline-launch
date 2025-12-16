#pragma once

#include "api/proto/video_service.grpc.pb.h"

namespace service::api {
    class IRequestHandler;

    class GrpcCallbackHandler final : public video::VideoService::CallbackService {
    public:
        explicit GrpcCallbackHandler(IRequestHandler& request_handler);

        grpc::ServerUnaryReactor* EnableOptionalElement(
            grpc::CallbackServerContext* context,
            const video::EnableOptionalElementRequest* request,
            video::EnableOptionalElementResponse* response) override;

        grpc::ServerUnaryReactor* DisableOptionalElement(
            grpc::CallbackServerContext* context,
            const video::DisableOptionalElementRequest* request,
            video::DisableOptionalElementResponse* response) override;

    private:
        IRequestHandler& request_handler_;
    };
}
