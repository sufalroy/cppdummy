#pragma once

#include "defines/compiler.h"
#include "defines/debug.h"
#include "defines/export.h"
#include "defines/platform.h"
#include "defines/types.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <variant>

#include <concepts>
#include <format>
#include <print>
#include <ranges>
#include <source_location>

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#if CPPDUMMY_PLATFORM_WINDOWS
#include <windows.h>
#endif

#if CPPDUMMY_PLATFORM_LINUX
#include <pthread.h>
#include <unistd.h>
#endif