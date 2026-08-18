#include "fatigue_service.h"



grpc::Status FatigueServiceImpl::ProcessBatch(grpc::ServerContext *context,
    const fatigue_worker::PressDataBatch *request, fatigue_worker::Fatigue *response) {
    if (request->samples().empty())
        return grpc::Status::OK;

    //TODO: Statelesness means this batch processing leads to a build up of inaccuracies as a swing could exist between batches
    ////This could be accepted or resolved in the aggregator.
    double reversalPosition = 0.0;
    int prevSign = 0;
    float fatigue = 0.0f; //Arbitrary, material-agnostic "Wear units"

    //ultra-simplified rainflow fatigue counting
    //Each half-cycle (velocity sign reversal) is weighted by distance travelled.
    //stop/start friction and stiction at zero-velocity are deliberately not modelled here.
    bool firstRun = true;
    for (const auto& sample : request->samples()) {
        const auto& velocity = sample.velocity();
        const auto& position = sample.position();
        const auto sign = GetSign(velocity);

        //Stop/start friction is deliberately left out of the model.
        if (sign == 0)
            continue;

        if (firstRun) {
            prevSign = GetSign(velocity);
            reversalPosition = position;
            firstRun = false;
        }

        if (sign != prevSign) {
            const double magnitude = std::pow(position - reversalPosition, 2);
            reversalPosition = position;
            fatigue += static_cast<float>(magnitude);
            prevSign = sign;
        }
    }
    response->set_batch_wear(fatigue);
    return grpc::Status::OK;
}
