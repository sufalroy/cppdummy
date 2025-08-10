#pragma once

#include "platform.h"

#ifdef CPPDUMMY_PLATFORM_WINDOWS
#define CPPDUMMY_EXPORT __declspec(dllexport)
#define CPPDUMMY_IMPORT __declspec(dllimport)
#else
#define CPPDUMMY_EXPORT __attribute__((visibility("default")))
#define CPPDUMMY_IMPORT __attribute__((visibility("default")))
#endif
