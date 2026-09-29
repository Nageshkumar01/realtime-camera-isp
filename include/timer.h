#pragma once

namespace isp {

// A stopwatch that measures elapsed time in microseconds.
// On Windows it uses QueryPerformanceCounter, which is precise enough for
// very small times. (std::chrono clocks on some MinGW versions only tick
// every few milliseconds, which would make small images show "0".)
class Timer {
public:
    Timer();
    void restart();
    double elapsedMicroseconds() const;

private:
    long long start_;  // clock ticks (Windows) or nanoseconds (other systems)
};

}  // namespace isp