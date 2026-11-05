#pragma once

#if defined(_WIN32)
    #if defined(SERVER_BUILD)
        #define SERVER_API __declspec(dllexport)
    #else
        #define SERVER_API __declspec(dllimport)
    #endif
#else
    #if defined(SERVER_BUILD)
        #define SERVER_API __attribute__((visibility("default")))
    #else
        #define SERVER_API
    #endif
#endif
