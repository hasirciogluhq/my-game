#pragma once

#if defined(_WIN32)
    #if defined(PHYSICS_BUILD)
        #define PHYSICS_API __declspec(dllexport)
    #else
        #define PHYSICS_API __declspec(dllimport)
    #endif
#else
    #if defined(PHYSICS_BUILD)
        #define PHYSICS_API __attribute__((visibility("default")))
    #else
        #define PHYSICS_API
    #endif
#endif
