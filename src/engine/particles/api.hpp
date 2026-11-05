#pragma once

#if defined(_WIN32)
    #if defined(PARTICLES_BUILD)
        #define PARTICLES_API __declspec(dllexport)
    #else
        #define PARTICLES_API __declspec(dllimport)
    #endif
#else
    #if defined(PARTICLES_BUILD)
        #define PARTICLES_API __attribute__((visibility("default")))
    #else
        #define PARTICLES_API
    #endif
#endif
