#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define CPPDUMMY_PLATFORM_WINDOWS 1
#define CPPDUMMY_PLATFORM_NAME "Windows"
#elif defined(__linux__)
#define CPPDUMMY_PLATFORM_LINUX 1
#define CPPDUMMY_PLATFORM_NAME "Linux"
#else
#error "Unsupported platform!"
#endif

#if CPPDUMMY_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
