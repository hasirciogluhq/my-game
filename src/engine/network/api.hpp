#pragma once

#if defined(_WIN32)
    #if defined(NETWORK_BUILD)
        #define NETWORK_API __declspec(dllexport)
    #else
        #define NETWORK_API __declspec(dllimport)
    #endif
#else
    #if defined(NETWORK_BUILD)
        #define NETWORK_API __attribute__((visibility("default")))
    #else
        #define NETWORK_API
    #endif
#endif
