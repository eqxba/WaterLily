#pragma once

#include <chrono>

class ScopeTimer
{
public:
    explicit ScopeTimer(std::string name = "ScopeTimer");
    ~ScopeTimer();

    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

    ScopeTimer(ScopeTimer&&) = delete;
    ScopeTimer& operator=(ScopeTimer&&) = delete;

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
};

namespace Helpers
{

}
