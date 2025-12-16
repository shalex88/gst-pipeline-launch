#pragma once

#include "api/proto/video_service.grpc.pb.h" //TODO: can move to implementation file?
#include "api/proto/video_service.pb.h"

namespace service::api {
    class IRequestHandler;

    class GrpcCallbackHandler final : public video::VideoService::CallbackService {
    public:
        explicit GrpcCallbackHandler(IRequestHandler& request_handler);

    private:
        IRequestHandler& request_handler_;
    };
}
