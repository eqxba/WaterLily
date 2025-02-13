#include "Utils/Helpers.hpp"

ScopeTimer::ScopeTimer(std::string aName /* = "ScopeTimer" */)
    : name{ std::move(aName) }
    , startTime{ std::chrono::high_resolution_clock::now() }
{}

ScopeTimer::~ScopeTimer()
{
    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    LogI << "[" << name << "] timer took " << duration.count() << " ms.\n";
}

namespace Helpers
{

}
