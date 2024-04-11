#pragma once

#ifdef _MSC_VER // MSVC
#define DISABLE_WARNINGS_BEGIN \
    __pragma(warning(push, 0))
#define DISABLE_WARNINGS_END \
    __pragma(warning(pop))
#elif defined(__clang__) // Clang
#define DISABLE_WARNINGS_BEGIN \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wall\"") \
    _Pragma("clang diagnostic ignored \"-Wextra\"") \
    _Pragma("clang diagnostic ignored \"-Wpedantic\"")
    _Pragma("clang diagnostic ignored \"-Wnullability-completeness\"")
#define DISABLE_WARNINGS_END \
    _Pragma("clang diagnostic pop")
#endif
