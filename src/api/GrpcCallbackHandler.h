#pragma once

#include "api/proto/video_service.grpc.pb.h"

namespace service::api {
    class IRequestHandler;

    class GrpcCallbackHandler final : public video::v1::VideoService::CallbackService {
    public:
        explicit GrpcCallbackHandler(IRequestHandler& request_handler);

        grpc::ServerUnaryReactor* SetVideoCapabilityState(
            grpc::CallbackServerContext* context,
            const video::v1::SetVideoCapabilityStateRequest* request,
            google::protobuf::Empty* response) override;

        grpc::ServerUnaryReactor* GetVideoCapabilities(
            grpc::CallbackServerContext* context,
            const google::protobuf::Empty* request,
            video::v1::GetVideoCapabilitiesResponse* response) override;

        grpc::ServerUnaryReactor* GetVideoCapabilityState(
            grpc::CallbackServerContext* context,
            const video::v1::GetVideoCapabilityStateRequest* request,
            video::v1::GetVideoCapabilityStateResponse* response) override;

    private:
        IRequestHandler& request_handler_;
    };
}
