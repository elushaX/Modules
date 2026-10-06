#pragma once

#include "Common.hpp"

#ifndef ENV_OS_WINDOWS
inline void* operator new(std::size_t aSize, void* aWhere) noexcept { return aWhere; }
inline void* operator new[](std::size_t aSize, void* aWhere) noexcept { return aWhere; }
#endif
