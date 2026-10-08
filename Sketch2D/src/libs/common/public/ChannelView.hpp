#pragma once

#include "Color.hpp"

#include <array>

namespace tp::sketch2d {

	enum class ViewChannel { Red, Green, Blue, AlphaOnBlack, AlphaOnWhite, Lightness };

	constexpr int4 kViewChannelCount = 6;
	constexpr halnf kLumaRed = 4899.0f / 16384.0f;
	constexpr halnf kLumaGreen = 9617.0f / 16384.0f;
	constexpr halnf kLumaBlue = 1868.0f / 16384.0f;

	using ChannelMatrix = std::array<halnf, 16>;

	[[nodiscard]] const char* viewChannelName(ViewChannel channel);
	[[nodiscard]] ChannelMatrix channelMatrix(ViewChannel channel);
	[[nodiscard]] RGBA applyChannelView(ViewChannel channel, const RGBA& premultiplied);
	[[nodiscard]] RGBA channelBackground(ViewChannel channel, const RGBA& background);
}
