#ifndef FATIGUEWORKER_FATIGUE_SERVICE_H
#define FATIGUEWORKER_FATIGUE_SERVICE_H
#include "fatigue.grpc.pb.h"


class FatigueServiceImpl final : public fatigue_worker::FatigueService::Service {
public:
    grpc::Status ProcessBatch(grpc::ServerContext *context, const fatigue_worker::PressDataBatch *request,
        fatigue_worker::Fatigue *response) override;

private:
    template <typename T>
    static int GetSign(T val) {
        return (T(0) < val) - (val < T(0));
    }
};


#endif //FATIGUEWORKER_FATIGUE_SERVICE_H
