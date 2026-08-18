#include <chrono>
#include <iostream>
#include <thread>
#include <csignal>
#include <future>
#include <grpcpp/grpcpp.h>

#include "press_data.h"
#include "press_sim_service.h"

constexpr int DEFAULT_TICK_INTERVAL_MS = 100;
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
        }
    }
}

static std::atomic<bool> running{true};
int main() {
    const PressData::PressConfig config;
    PressData::PressState state;
    const auto tickInterval = std::chrono::milliseconds(GetFromEnvOrDefault("PRESS_TICK_INTERVAL_MS", DEFAULT_TICK_INTERVAL_MS));

    auto ShutdownHandler = [] (int _) {
        running = false;
    };
    std::signal(SIGINT, ShutdownHandler);
    std::signal(SIGTERM, ShutdownHandler);

    PressSimServiceImpl grpcService(config, state, tickInterval, running);
    std::promise<grpc::Server*> serverPromise;
    std::future<grpc::Server*> serverFuture = serverPromise.get_future();
    std::thread serverThread([&serverPromise, &grpcService] {
        //This thread starts the gRPC server. It'll keep running on Wait() and listen for calls to the functions in press_sim_service.cpp.
        //We need to kill the server from outside the thread (to exit Wait()).
        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
        builder.RegisterService(&grpcService);
        const std::unique_ptr server(builder.BuildAndStart());
        if (!server) {
            std::cerr << "Failed to start server" << std::endl;
            serverPromise.set_value(nullptr);
            return;
        }
        serverPromise.set_value(server.get());
        server->Wait();
    });

    grpc::Server* server = serverFuture.get();
    if (!server) {
        running = false;
    }

    const double dt = std::chrono::duration<double>(tickInterval).count();
    while (running) {
        state.mutex.lock();
        RunCycle(config, state, dt);
        state.mutex.unlock();

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
