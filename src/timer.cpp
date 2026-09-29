#include "timer.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace isp {

namespace {

long long nowTicks() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

double ticksPerSecond() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return static_cast<double>(frequency.QuadPart);
}

}  // namespace

Timer::Timer() : start_(nowTicks()) {}

void Timer::restart() { start_ = nowTicks(); }

double Timer::elapsedMicroseconds() const {
    return static_cast<double>(nowTicks() - start_) * 1000000.0 / ticksPerSecond();
}

}  // namespace isp

#else  // Linux / other systems

#include <chrono>

namespace isp {

namespace {

long long nowNanoseconds() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

}  // namespace

Timer::Timer() : start_(nowNanoseconds()) {}

void Timer::restart() { start_ = nowNanoseconds(); }

double Timer::elapsedMicroseconds() const {
    return static_cast<double>(nowNanoseconds() - start_) / 1000.0;
}

}  // namespace isp

#endif