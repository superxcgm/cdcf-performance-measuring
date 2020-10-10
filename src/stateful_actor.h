#ifndef CDCF_PERFORMANCE_MEASURING_STATEFUL_ACTOR_H
#define CDCF_PERFORMANCE_MEASURING_STATEFUL_ACTOR_H

#include <caf/all.hpp>

struct ActorState {
    int i;
};

caf::behavior countActorFun(caf::stateful_actor<ActorState> *self, CountDownLatch *finish_latch, int n) {
    self->state.i = n;
    return {
            [=](EmptyMessage &emptymessage) {
                self->state.i--;
                if (self->state.i == 0) {
                    finish_latch->countDown();
                    self->quit();
                }
            }
    };
}

#endif //CDCF_PERFORMANCE_MEASURING_STATEFUL_ACTOR_H
