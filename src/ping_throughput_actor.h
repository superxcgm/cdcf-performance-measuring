#ifndef CDCF_PERFORMANCE_MEASURING_PING_THROUGHPUT_ACTOR_H
#define CDCF_PERFORMANCE_MEASURING_PING_THROUGHPUT_ACTOR_H

#include <caf/all.hpp>
#include "./count_down_latch.h"

struct PingThroughputActorState {
    int i;
};

struct PingThroughputMessage {
    caf::actor sender;
};

template<class Inspector>
typename Inspector::result_type inspect(Inspector &f, const PingThroughputMessage &x) {
    return f(caf::meta::type_name("PingThroughputMessage"));
}

caf::behavior pingThroughputActorFun(caf::stateful_actor<PingThroughputActorState> *self,
                                     CountDownLatch *finish_latch, int n) {
    self->state.i = n;
    return {
            [=](PingThroughputMessage &pingthroughputmessage) {
                if (self->state.i > 0 && pingthroughputmessage.sender != nullptr) {
                    caf::actor new_sender = nullptr;
                    if (self->state.i > 1) {
                        new_sender = self;
                    }
                    PingThroughputMessage new_message{new_sender};
                    self->send(pingthroughputmessage.sender, new_message);
                }
                self->state.i--;
                if (self->state.i == 0) {
                    finish_latch->countDown();
                    caf::anon_send(self, caf::exit_reason::kill);
                }
            }
    };
}

#endif //CDCF_PERFORMANCE_MEASURING_PING_THROUGHPUT_ACTOR_H
