#include "Timer.h"
#include "Platform.h"

namespace Nexus {

Timer::Timer() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    frequency_ = freq.QuadPart;
    Reset();
}

Timer::~Timer() = default;

void Timer::Reset() {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    startTime_ = time.QuadPart;
}

float Timer::GetElapsedTime() const {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    return static_cast<float>(currentTime.QuadPart - startTime_) / frequency_;
}

} // namespace Nexus
