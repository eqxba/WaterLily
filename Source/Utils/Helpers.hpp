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
    template<typename Func, typename Vector, typename... Args>
    auto Transform(Func&& func, const Vector& input, Args&&... args);
}

template<typename Func, typename Vector, typename... Args>
auto Helpers::Transform(Func&& func, const Vector& input, Args&&... args)
{
    using InType = typename Vector::value_type;
    using OutType = std::invoke_result_t<Func, const InType&, Args...>;

    std::vector<OutType> output;
    output.reserve(input.size());

    for (const auto& elem : input)
    {
        output.emplace_back(std::invoke(std::forward<Func>(func), elem, std::forward<Args>(args)...));
    }

    return output;
}
