#include "fatigue_service.h"

/* NOTE: ** Lesson worth documenting for future reference **
Initially this used a highly simplified rainflow counting algorithm to more closely mimick real fatigue tracking.
It means fatigue is measured at each reversal about the squared displacement during the prior half-cycle.
A worker doesn't need state to process a batch like this. We can have many worker pods and each can do their job and stand in for one another.
However, when designing for parallelism, what's also important is that our input and output is stateless:
In this case, some state ended up being relevant for the batch: it could be mid-swing, and then relevant data would be hidden somewhere in a previous batch, or lost between batches.
In fact, this problem is common enough, that the absence of that problem is known as "Embarrassingly parallel".
Resolving that would require trickery that would distract from the goal of this project, but it shows it's something you'll often need to account for.
Bottom line: Statelessness of a worker isn't a property of the worker in isolation that doesn't keep state, but describes its relationship to the surrounding system.
*/

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
