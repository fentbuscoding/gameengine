#pragma once

namespace Nexus {

class Timer {
public:
    Timer();
    ~Timer();
    
    void Reset();
    float GetElapsedTime() const;
    
private:
    long long frequency_;
    long long startTime_;
};

} // namespace Nexus
