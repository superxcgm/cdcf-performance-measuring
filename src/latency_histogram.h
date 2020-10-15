#ifndef CDCF_PERFORMANCE_MEASURING_LATENCY_HISTOGRAM_H
#define CDCF_PERFORMANCE_MEASURING_LATENCY_HISTOGRAM_H

#include <hdr_histogram.h>
#include <atomic>
#include <chrono>

class LatencyHistogram {
public:
    LatencyHistogram() {
        hdr_init(1, INT64_C(1000'000'000), 2, &latency_histogram_);
    }

    void Record() {
        auto t1 = std::chrono::high_resolution_clock::now();

        if (t0_.time_since_epoch().count() != 0) {
            hdr_record_value(latency_histogram_,
                             std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0_).count());
        }
        t0_ = t1;
    }

    struct hdr_histogram *latency_histogram_{};
    // std::chrono::time_point<std::chrono::steady_clock> t0_;
    std::chrono::time_point<std::chrono::high_resolution_clock> t0_;
};

#endif //CDCF_PERFORMANCE_MEASURING_LATENCY_HISTOGRAM_H
