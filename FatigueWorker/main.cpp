
#include <atomic>
#include <csignal>
#include <thread>
#include <iostream>
#include <cmath>
#include <grpcpp/grpcpp.h>

#include "fatigue_service.h"


static std::atomic<bool> running{true};


int main() {
    auto ShutdownHandler = [] (int _) {
        running = false;
    };
    std::signal(SIGINT, ShutdownHandler);
    std::signal(SIGTERM, ShutdownHandler);

    FatigueServiceImpl fatigueService;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50052", grpc::InsecureServerCredentials());
    builder.RegisterService(&fatigueService);
    const std::unique_ptr server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "Failed to start server" << std::endl;
        running = false;
    }

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (server)
        server->Shutdown();
    return 0;
}
