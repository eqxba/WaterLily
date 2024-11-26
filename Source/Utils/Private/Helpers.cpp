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
    std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        const bool isFileOpened = file.is_open();
        Assert(isFileOpened);

        const auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}