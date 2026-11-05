#pragma once

#if defined(_WIN32)
    #if defined(UI_BUILD)
        #define UI_API __declspec(dllexport)
    #else
        #define UI_API __declspec(dllimport)
    #endif
#else
    #if defined(UI_BUILD)
        #define UI_API __attribute__((visibility("default")))
    #else
        #define UI_API
    #endif
#endif
