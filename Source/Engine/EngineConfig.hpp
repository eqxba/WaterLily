#pragma once

#include "Engine/Window.hpp"

// Struct instead of constant values in a namespace to make configs easy swappable? Don't care rn
namespace EngineConfig
{
    constexpr const char *engineName = "WaterLily";

    constexpr WindowDescription windowDescription = {
        .extent = { 1980 / 2, 1080 / 2},
        .title = engineName,
        .mode = WindowMode::eWindowed,
        .inputMode = InputMode::eEngine };

    constexpr std::string_view defaultScenePath = "~/Assets/Scenes/Duck/Duck.glb";

    // Feature to copy the whole scene with random transforms many times to reach significant amount of issued draws
    constexpr bool randomlyCopyScene = true;
}
