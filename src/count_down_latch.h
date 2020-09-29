#ifndef CDCF_PERFORMANCE_MEASURING_COUNT_DOWN_LATCH_H
#define CDCF_PERFORMANCE_MEASURING_COUNT_DOWN_LATCH_H

#include <chrono>
#include <condition_variable>
#include <mutex>

class CountDownLatch {
public:
    explicit CountDownLatch(const unsigned int count) : m_count(count) {}

    virtual ~CountDownLatch() = default;

    void await() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count > 0) {
            m_cv.wait(lock, [this]() { return m_count == 0; });
        }
    }

    template<class Rep, class Period>
    bool await(const std::chrono::duration<Rep, Period> &timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool result = true;
        if (m_count > 0) {
            result = m_cv.wait_for(lock, timeout, [this]() { return m_count == 0; });
        }

        return result;
    }

    void countDown() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count > 0) {
            m_count--;
            m_cv.notify_all();
        }
    }

    unsigned int getCount() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_count;
    }

protected:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    unsigned int m_count = 0;
};

#endif //CDCF_PERFORMANCE_MEASURING_COUNT_DOWN_LATCH_H
