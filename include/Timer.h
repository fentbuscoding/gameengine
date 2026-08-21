#pragma once

#include <cstdint>

namespace Nexus {

/**
 * Monotonic high-resolution timer.
 *
 * Backed by std::chrono::steady_clock, so elapsed time never goes backwards
 * even if the system wall clock is adjusted mid-frame.
 */
class Timer {
public:
    Timer();
    ~Timer();

    /// Restarts both the elapsed-time origin and the per-tick delta baseline.
    void Reset();

    /// Seconds since the last Reset().
    float GetElapsedTime() const;

    /// Seconds since the last Reset(), at full double precision. Prefer this
    /// for long-running accumulators, where float loses resolution after a few
    /// hours of uptime.
    double GetElapsedSeconds() const;

    /// Seconds since the previous Tick() (or Reset()), and advances the
    /// baseline. This is the per-frame delta time.
    float Tick();

private:
    std::int64_t frequency_;     ///< Ticks per second (nanosecond resolution).
    std::int64_t startTime_;     ///< Origin for GetElapsedTime().
    std::int64_t lastTickTime_;  ///< Baseline for Tick().
};

} // namespace Nexus
