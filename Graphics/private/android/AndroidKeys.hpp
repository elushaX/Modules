#pragma once

#include "InputCodes.hpp"

#include <cstdint>
#include <optional>

namespace tp::android {

	[[nodiscard]] std::optional<InputID> inputFromKeyCode(int32_t keyCode);
}
