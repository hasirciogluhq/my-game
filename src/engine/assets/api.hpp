#pragma once

#if defined(_WIN32)
    #if defined(ASSETS_BUILD)
        #define ASSETS_API __declspec(dllexport)
    #else
        #define ASSETS_API __declspec(dllimport)
    #endif
#else
    #if defined(ASSETS_BUILD)
        #define ASSETS_API __attribute__((visibility("default")))
    #else
        #define ASSETS_API
    #endif
#endif
