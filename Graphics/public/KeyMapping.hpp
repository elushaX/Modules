#pragma once

#include "InputCodes.hpp"

#include <cstdint>
#include <optional>

namespace tp {

	[[nodiscard]] std::optional<InputID> inputFromEvdevKey(uint32_t evdevKey);

	[[nodiscard]] std::optional<InputID> inputFromPointerButton(uint32_t linuxButton);
}
