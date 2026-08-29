#include "fatigue_service.h"

//Wear increases exponentially with movement speed, disregarding distance travelled.
//This approach measures per two adjacent samples, which is easier to scale horizontally (see above note).
grpc::Status FatigueServiceImpl::ProcessBatch(grpc::ServerContext *context,
    const fatigue_worker::PressDataBatch *request, fatigue_worker::Fatigue *response) {
    if (request->samples().empty())
        return grpc::Status::OK;

    double fatigue = 0;
    const auto& samples = request->samples();

    if (request->has_prev_sample()) {
        fatigue += std::pow(samples[0].position() - request->prev_batch_last_sample().position(), 2);
    }
    for(int i = 1; i < samples.size(); ++i) {
        fatigue += std::pow(samples[i].position() - samples[i - 1].position(), 2);
    }

    response->set_batch_wear(fatigue);
    return grpc::Status::OK;
}
