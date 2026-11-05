#pragma once

#if defined(_WIN32)
#if defined(CLIENT_BUILD)
#define CLIENT_API __declspec(dllexport)
#else
#define CLIENT_API __declspec(dllimport)
#endif
#else
#if defined(CLIENT_BUILD)
#define CLIENT_API __attribute__((visibility("default")))
#else
#define CLIENT_API
#endif
#endif
