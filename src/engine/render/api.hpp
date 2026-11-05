#pragma once

#if defined(_WIN32)
    #if defined(RENDER_BUILD)
        #define RENDER_API __declspec(dllexport)
    #else
        #define RENDER_API __declspec(dllimport)
    #endif
#else
    #if defined(RENDER_BUILD)
        #define RENDER_API __attribute__((visibility("default")))
    #else
        #define RENDER_API
    #endif
#endif
