#pragma once

#include "PublicConfig.hpp"

#define MEM_WRAP_SIZE 8               // Wrapping Size in aligned units
#define MEM_WRAP_FILL_VAL 0xBB        // Wrapping Fill Value
#define MEM_CLEAR_ON_ALLOC            // Clear data on allocation
#define MEM_CLEAR_ON_DEALLOC          // Clear data on free
#define MEM_CLEAR_ON_DEALLOC_VAL 0xAA // Clear data on free
#define MEM_CLEAR_ON_ALLOC_VAL 0xCC   // Clear data on free
#define MEM_STACK_TRACE               // Save stack on allocation call
#define MEM_STACK_TRACE_MAX_DEPTH 32  // Call stack max depth