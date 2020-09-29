#ifndef CDCF_PERFORMANCE_MEASURING_BLOCKABLE_COUNT_ACTOR_H
#define CDCF_PERFORMANCE_MEASURING_BLOCKABLE_COUNT_ACTOR_H

#include <caf/all.hpp>
#include "./count_down_latch.h"

struct BlockableCountActorState {
    int blocked = true;
    int i;
};

struct EmptyMessage {

};

template<class Inspector>
typename Inspector::result_type inspect(Inspector &f, const EmptyMessage &x) {
    return f(caf::meta::type_name("EmptyMessage"));
}

caf::behavior blockableCountActorFun(caf::stateful_actor<BlockableCountActorState> *self, CountDownLatch *start_latch,
                                     CountDownLatch *finish_latch, int n) {
    self->state.i = n - 1;
    return {
            [=](EmptyMessage &emptymessage) {
                if (self->state.blocked) {
                    start_latch->await();
                    self->state.blocked = false;
                } else {
//                    std::cout << "i: " << self->state.i << std::endl;
                    self->state.i--;
                    if (self->state.i == 0) {
                        finish_latch->countDown();
                        caf::anon_send(self, caf::exit_reason::kill);
                    }
                }
            }
    };
}

#endif //CDCF_PERFORMANCE_MEASURING_BLOCKABLE_COUNT_ACTOR_H
