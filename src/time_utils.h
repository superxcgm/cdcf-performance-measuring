//
// Created by Yuecheng Pei on 2020/9/30.
//

#ifndef CDCF_PERFORMANCE_MEASURING_TIME_UTILS_H
#define CDCF_PERFORMANCE_MEASURING_TIME_UTILS_H

#include<iostream>
#include<string>
#include<chrono>
#include<thread>

enum class TimeUnit {
    MilliSeconds, MicroSeconds, Seconds, NanoSeconds, Minutes, Hours
};

class TimeUtils {
public:
    static void customSleep(const long &quantity, const TimeUnit &unit) {
        switch (unit) {
            case TimeUnit::MilliSeconds :
                std::this_thread::sleep_for(std::chrono::milliseconds(quantity));
                break;
            case TimeUnit::MicroSeconds :
                std::this_thread::sleep_for(std::chrono::microseconds(quantity));
                break;
            case TimeUnit::Seconds :
                std::this_thread::sleep_for(std::chrono::seconds(quantity));
                break;
            case TimeUnit::NanoSeconds :
                std::this_thread::sleep_for(std::chrono::nanoseconds(quantity));
                break;
            case TimeUnit::Minutes :
                std::this_thread::sleep_for(std::chrono::minutes(quantity));
                break;
            case TimeUnit::Hours :
                std::this_thread::sleep_for(std::chrono::hours(quantity));
                break;
            default:
                std::cout << "unknown TimeUnit type. Please verify." << std::endl;
                break;
        }
    }

    static std::chrono::milliseconds waitDuration(const long &quantity, const TimeUnit &unit) {
        // This method converts any given quantity and TimeUnit type to corrosponding millisecond wait period.
        std::chrono::milliseconds returnValue;
        switch (unit) {
            case TimeUnit::MilliSeconds :
                returnValue = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::milliseconds(quantity));
                break;
            case TimeUnit::MicroSeconds :
                returnValue = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::microseconds(quantity));
                break;
            case TimeUnit::Seconds :
                returnValue = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(quantity));
                break;
            case TimeUnit::NanoSeconds :
                returnValue = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(quantity));
                break;
            case TimeUnit::Minutes :
                returnValue = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(quantity));
                break;
            case TimeUnit::Hours :
                returnValue = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(quantity));
                break;
        }

        return (returnValue);
    }
};


#endif //CDCF_PERFORMANCE_MEASURING_TIME_UTILS_H
