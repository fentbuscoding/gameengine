#include "Timer.h"

#include <chrono>

namespace Nexus {

// Timer previously called QueryPerformanceCounter directly, which pinned it to
// Win32. std::chrono::steady_clock gives the same guarantee that mattered here -
// a monotonic counter that never jumps when the wall clock is adjusted - on
// every supported platform, so the Win32 dependency bought nothing.

namespace {

// steady_clock is the right choice over high_resolution_clock: the latter is
// permitted to alias system_clock, which can step backwards when NTP corrects
// the wall clock and would produce negative frame times.
using Clock = std::chrono::steady_clock;

std::int64_t NowTicks() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch())
        .count();
}

constexpr std::int64_t kTicksPerSecond = 1000000000;

} // namespace

Timer::Timer() {
    frequency_ = kTicksPerSecond;
    Reset();
}

Timer::~Timer() = default;

void Timer::Reset() {
    startTime_ = NowTicks();
    lastTickTime_ = startTime_;
}

float Timer::GetElapsedTime() const {
    return static_cast<float>(
        static_cast<double>(NowTicks() - startTime_) / static_cast<double>(kTicksPerSecond));
}

double Timer::GetElapsedSeconds() const {
    return static_cast<double>(NowTicks() - startTime_) / static_cast<double>(kTicksPerSecond);
}

float Timer::Tick() {
    const std::int64_t now = NowTicks();
    const std::int64_t delta = now - lastTickTime_;
    lastTickTime_ = now;
    return static_cast<float>(static_cast<double>(delta) / static_cast<double>(kTicksPerSecond));
}

} // namespace Nexus
