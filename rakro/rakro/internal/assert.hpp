#pragma once
#ifdef _WIN32
#include <assert.h>
#include <intrin.h>

#endif

#ifdef _DEBUG
#define rakro_assert(...)                                                                      \
    {                                                                                          \
        const bool success_##__LINE__ = (__VA_ARGS__);                                         \
        if (!success_##__LINE__) {                                                             \
            __debugbreak();                                                                    \
        }                                                                                      \
        assert(false);                                                                         \
    }
#else
#define rakrakro_assert(...)
#endif