#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <string_view>
#include <iostream>
#include <algorithm>
#include <execution>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <limits>
#include <fstream>
#include <unordered_map>
#include <typeindex>
#include <any>
#include <functional>
#include <chrono>
#include <numeric>
#include <ranges>

#include "Utils/Assert.hpp"
#include "Utils/Logger.hpp"

#ifdef PLATFORM_WIN
    constexpr bool platformWin = true;
#else
    constexpr bool platformWin = false;
#endif

#ifdef PLATFORM_MAC
    constexpr bool platformMac = true;
#else
    constexpr bool platformMac = false;
#endif

#ifdef _MSC_VER // MSVC

#pragma warning(disable: 4100) // Unreferenced formal parameter
#pragma warning(disable: 4702) // Unreachable code
#pragma warning(disable: 4505) // Unreferenced local function has been removed

#elif defined(__clang__) // Clang

#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wunused-function"

#endif
