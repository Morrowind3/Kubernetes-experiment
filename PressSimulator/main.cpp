#include <chrono>
#include <iostream>
#include <thread>
#include <csignal>

namespace {
    enum class State {
        Extend,
        Hold,
        Retract,
    };
    template<typename T>
    T GetFromEnvOrDefault(const char* envName, T defaultValue) {
        if (const char* value = std::getenv(envName)) {
            try {
                return std::stod(value);
            } catch (std::exception&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    struct PressData {
        PressData() :
        position(0.0),
        velocity(0.0),
        maxDepth(GetFromEnvOrDefault("PRESS_MAX_DEPTH", 500.0)),
        speed(GetFromEnvOrDefault("PRESS_SPEED", 50.0)),
        dwellTicks(0), dwellDuration(GetFromEnvOrDefault("PRESS_DWELL_DURATION", 20)),
        state(State::Extend){}

        double position;       // mm, 0 = fully retracted (home)
        double velocity;       // mm/s,
        double maxDepth;     // mm
        double speed;         // mm/s
        uint32_t dwellTicks;          // how many ticks we've been dwelling so far
        uint32_t dwellDuration;       // ticks to dwell before retracting
        State state;
    };

    void RunCycle(PressData& press, double dt) {
        switch (press.state) {
            case State::Extend:
                press.velocity = press.speed;
                press.position += press.velocity * dt;
                if (press.position >= press.maxDepth) {
                    press.position = press.maxDepth;
                    press.state = State::Hold;
                }
                break;

            case State::Hold:
                press.velocity = 0;
                if (++press.dwellTicks >= press.dwellDuration) {
                    press.dwellTicks = 0;
                    press.state = State::Retract;
                };
                break;

            case State::Retract:
                press.velocity = -press.speed;
                press.position += press.velocity * dt;
                if (press.position <= 0) {
                    press.position = 0;
                    press.state = State::Extend;
                }
                break;
        }
    }

    const char* StateToString(State state) {
        switch (state) {
            case State::Extend:  return "EXTEND";
            case State::Hold:    return "HOLD";
            case State::Retract: return "RETRACT";
        }
        return "UNKNOWN";
    }
}

static std::atomic<bool> running{true};
int main() {
    PressData data;

    const auto tickInterval = std::chrono::milliseconds(GetFromEnvOrDefault("PRESS_TICK_INTERVAL_MS", 100));
    const double dt = std::chrono::duration<double>(tickInterval).count();
    auto ShutdownHandler = [] (int _) {
        running = false;
    };

    std::signal(SIGINT, ShutdownHandler);
    std::signal(SIGTERM, ShutdownHandler);
    while (running) {
        RunCycle(data, dt);

        std::cout << "[" << StateToString(data.state) << "] "
                  << "position=" << data.position << "mm "
                  << "velocity=" << data.velocity << "mm/s"
                  << std::endl;

        std::this_thread::sleep_for(tickInterval);
    }

    return 0;
}
