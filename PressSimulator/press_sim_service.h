#ifndef PRESSSIMULATOR_GRPCSERVICE_H
#define PRESSSIMULATOR_GRPCSERVICE_H

#include <grpcpp/grpcpp.h>
#include "press.grpc.pb.h"

namespace PressData {
    struct PressConfig;
    struct PressState;
}

//Protobuf generates the client implementation (Stub/StubInterface) and handles all the plumbing.
//Implementing the service is our responsibility and makes the right data passes through the plumbing.
class PressSimServiceImpl final : public press_simulator::PressSimService::Service {
public:
    PressSimServiceImpl(const PressData::PressConfig& pressConfig, PressData::PressState& pressState, const std::chrono::milliseconds tickIntervalMs, const bool& applicationRunning)
        : _pressState(pressState), _pressConfig(pressConfig), _tickIntervalMs(tickIntervalMs), _applicationRunning(applicationRunning){
    };

    grpc::Status FetchMetrics(grpc::ServerContext *context,
                              const google::protobuf::Empty *request,
                              grpc::ServerWriter<press_simulator::PressMetrics> *writer) override;

    grpc::Status FetchConfig(grpc::ServerContext* context,
                            const google::protobuf::Empty* request,
                            press_simulator::PressConfig* response) override;
private:
    PressData::PressState& _pressState;
    const PressData::PressConfig& _pressConfig;
    const std::chrono::milliseconds _tickIntervalMs;
    const bool& _applicationRunning;
};
#endif //PRESSSIMULATOR_GRPCSERVICE_H
