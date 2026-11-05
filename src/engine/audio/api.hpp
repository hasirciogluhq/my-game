#pragma once

#if defined(_WIN32)
    #if defined(AUDIO_BUILD)
        #define AUDIO_API __declspec(dllexport)
    #else
        #define AUDIO_API __declspec(dllimport)
    #endif
#else
    #if defined(AUDIO_BUILD)
        #define AUDIO_API __attribute__((visibility("default")))
    #else
        #define AUDIO_API
    #endif
#endif
