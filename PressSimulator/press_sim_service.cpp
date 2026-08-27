#include "press_sim_service.h"
#include <thread>
#include "press_data.h"


grpc::Status PressSimServiceImpl::FetchMetrics(grpc::ServerContext *context, const google::protobuf::Empty*,
                                               grpc::ServerWriter<press_simulator::PressMetrics> *writer) {
    press_simulator::PressMetrics metrics;
    while (!context->IsCancelled() && _applicationRunning) {
        _pressState.mutex.lock();
        metrics.set_position(_pressState.position);
        metrics.set_velocity(_pressState.velocity);
        switch (_pressState.state) {
            case PressData::State::Extend:
                metrics.set_state(press_simulator::PressState::EXTEND);
                break;
            case PressData::State::Hold:
                metrics.set_state(press_simulator::PressState::HOLD);
                break;
            case PressData::State::Retract:
                metrics.set_state(press_simulator::PressState::RETRACT);
                break;
        }
        _pressState.mutex.unlock();
        writer->Write(metrics);
        std::this_thread::sleep_for(_tickIntervalMs);
    }
    return grpc::Status::OK;
}

grpc::Status PressSimServiceImpl::FetchConfig(grpc::ServerContext*, const google::protobuf::Empty*,
    press_simulator::PressConfig *response) {
    response->set_dwell_duration(_pressConfig.dwellDuration);
    response->set_max_depth(_pressConfig.maxDepth);
    response->set_speed(_pressConfig.speed);

    return grpc::Status::OK;
}
