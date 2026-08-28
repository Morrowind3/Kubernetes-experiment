#include <chrono>
#include <iostream>
#include <thread>
#include <csignal>
#include <future>
#include <grpcpp/grpcpp.h>

#include <prometheus/gauge.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "press_data.h"
#include "press_sim_service.h"

constexpr int DEFAULT_TICK_INTERVAL_MS = 100;
static std::atomic<bool> running{true};

namespace {
    void RunCycle(const PressData::PressConfig& pressConfig, PressData::PressState& pressState, const double dt) {
        switch (pressState.state) {
            case PressData::State::Extend:
                pressState.velocity = pressConfig.speed;
                pressState.position += pressState.velocity * dt;
                if (pressState.position >= pressConfig.maxDepth) {
                    pressState.position = pressConfig.maxDepth;
                    pressState.state = PressData::State::Hold;
                }
                break;

            case PressData::State::Hold:
                pressState.velocity = 0;
                if (++pressState.dwellTicks >= pressConfig.dwellDuration) {
                    pressState.dwellTicks = 0;
                    pressState.state = PressData::State::Retract;
                };
                break;

            case PressData::State::Retract:
                pressState.velocity = -pressConfig.speed;
                pressState.position += pressState.velocity * dt;
                if (pressState.position <= 0) {
                    pressState.position = 0;
                    pressState.state = PressData::State::Extend;
                }
                break;
        };
    };

    //This thread starts the gRPC server. It'll keep running on Wait() and listen for calls to the functions in press_sim_service.cpp.
    void grpcServerFunc(std::promise<grpc::Server*>&& serverPromise, const PressData::PressConfig& config, PressData::PressState& state, const std::chrono::milliseconds& tickInterval) {
            PressSimServiceImpl grpcService(config, state, tickInterval, running);
            grpc::ServerBuilder builder;
            //"InsecureServerCredentials" means no TLS. It's fine here as it's not exposed (ClusterIP) and the data isn't sensitive. 
            builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials()); 
            builder.RegisterService(&grpcService);
            const std::unique_ptr server(builder.BuildAndStart());
            if (!server) {
                std::cerr << "Failed to start server" << std::endl;
                serverPromise.set_value(nullptr);
                return;
            }
            serverPromise.set_value(server.get());
            //Blocking, we need to kill the server from outside the thread.
            server->Wait();
    }
};

int main() {
    const PressData::PressConfig config;
    PressData::PressState state;
    const auto tickInterval = std::chrono::milliseconds(GetFromEnvOrDefault("PRESS_TICK_INTERVAL_MS", DEFAULT_TICK_INTERVAL_MS));

    auto ShutdownHandler = [] (int _) {
        running = false;
    };
    std::signal(SIGINT, ShutdownHandler);
    std::signal(SIGTERM, ShutdownHandler);

    std::promise<grpc::Server*> mv_serverPromise;
    std::future<grpc::Server*> serverFuture = mv_serverPromise.get_future();
    std::thread serverThread(grpcServerFunc, std::move(mv_serverPromise), std::ref(config), std::ref(state), tickInterval);

    grpc::Server* server = serverFuture.get();
    if (!server) {
        running = false; //TODO: Exit error codes
    }

    //prometheus server
    prometheus::Exposer exposer{"0.0.0.0:9090"};
    std::shared_ptr<prometheus::Registry> registry = std::make_shared<prometheus::Registry>();

    auto& positionGauge = prometheus::BuildGauge()
                              .Name("press_position_mm")
                              .Help("Current position of the press in mm")
                              .Register(*registry)
                              .Add({});
    auto& velocityGauge = prometheus::BuildGauge()
                              .Name("press_velocity_mm_per_s")
                              .Help("Current velocity of the press in mm/s")
                              .Register(*registry)
                              .Add({});
    auto& stateGauge = prometheus::BuildGauge()
                              .Name("press_state")
                              .Help("Current state of the press (0=Extend, 1=Hold, 2=Retract)")
                              .Register(*registry)
                              .Add({});
    
    exposer.RegisterCollectable(registry);


    const double dt = std::chrono::duration<double>(tickInterval).count();
    while (running) {
        {
            std::scoped_lock lock(state.mutex);
            RunCycle(config, state, dt);
            positionGauge.Set(state.position);
            velocityGauge.Set(state.velocity);
            stateGauge.Set(static_cast<double>(static_cast<uint8_t>(state.state)));
        }

        std::cout << "[" << StateToString(state.state) << "] "
                  << "position=" << state.position << "mm "
                  << "velocity=" << state.velocity << "mm/s"
                  << std::endl;

        std::this_thread::sleep_for(tickInterval);
    }

    if (server)
        server->Shutdown();
    serverThread.join();

    return 0;
}
