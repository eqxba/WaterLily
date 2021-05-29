#pragma once

#include <cassert>

// TODO: Constexpr template function?
#ifdef NDEBUG
#define Assert(expression) if (!(expression)) \
	{ std::cout << "Assertion failed: " << #expression << ", file " << __FILE__ << ", line " << __LINE__ << std::endl; }
#else
#define Assert(expression) assert(expression)
#endif