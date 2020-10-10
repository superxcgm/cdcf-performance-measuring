#ifndef CDCF_PERFORMANCE_MEASURING_PING_LATENCY_ACTOR_H
#define CDCF_PERFORMANCE_MEASURING_PING_LATENCY_ACTOR_H

#include <caf/all.hpp>
#include "./count_down_latch.h"

struct PingLatencyActorState {
    int i;
};

struct PingLatencyMessage {
    caf::actor sender;
};

template<class Inspector>
typename Inspector::result_type inspect(Inspector &f, const PingLatencyMessage &x) {
    return f(caf::meta::type_name("PingThroughputMessage"));
}

caf::behavior pingLatencyActorFun(caf::stateful_actor<PingLatencyActorState> *self,
                                     CountDownLatch *finish_latch, int n) {
    self->state.i = n;
    return {
            [=](PingLatencyMessage &pinglatencymessage) {
                //todo: record
                if (self->state.i > 0 && pinglatencymessage.sender != nullptr) {
                    caf::actor new_sender = nullptr;
                    if (self->state.i > 1) {
                        new_sender = self;
                    }
                    PingLatencyMessage new_message{new_sender};
                    self->send(pinglatencymessage.sender, new_message);
                }
                self->state.i--;
                if (self->state.i == 0) {
                    finish_latch->countDown();
                    caf::anon_send(self, caf::exit_reason::kill);
                }
            }
    };
}

#endif //CDCF_PERFORMANCE_MEASURING_PING_LATENCY_ACTOR_H
