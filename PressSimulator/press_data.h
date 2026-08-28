#ifndef PRESSSIMULATOR_PRESSDATA_H
#define PRESSSIMULATOR_PRESSDATA_H

#include <mutex>

#include "utils.h"

namespace PressData {
    enum class State : uint8_t {
        Extend,
        Hold,
        Retract,
    };

    inline const char* StateToString(State state) {
        switch (state) {
            case State::Extend:  return "EXTEND";
            case State::Hold:    return "HOLD";
            case State::Retract: return "RETRACT";
        }
        return "UNKNOWN";
    }
    //Constantly updated data; Needs mutex protection.
    struct PressState {
        PressState() : position(0.0), velocity(0.0), state(State::Extend), dwellTicks(0){

        }
        double position;       // mm, 0 = fully retracted
        double velocity;       // mm/s,
        uint32_t dwellTicks;   // how many ticks we've been dwelling so far
        State state;
        std::mutex mutex;
    };

    struct PressConfig {
        PressConfig() :
        maxDepth(GetFromEnvOrDefault("PRESS_MAX_DEPTH", 500.0)),
        speed(GetFromEnvOrDefault("PRESS_SPEED", 50.0)),
        dwellDuration(GetFromEnvOrDefault("PRESS_DWELL_DURATION", 20))
        {}

        const double maxDepth;          // mm
        const double speed;             // mm/s
        const uint32_t dwellDuration;   // ticks to dwell before retracting
    };
}


#endif //PRESSSIMULATOR_PRESSDATA_H
