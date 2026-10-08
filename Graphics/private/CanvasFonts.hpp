#pragma once

#include <array>

namespace tp {

	struct FontFile {
		const char* name;
		const char* path;
	};

	inline constexpr FontFile kRegularFont = { "default", "rsc/Font.ttf" };
	inline constexpr FontFile kBoldFont = { "bold", "rsc/FontBold.ttf" };
	inline constexpr FontFile kMonospaceFont = { "monospace", "rsc/FontMono.ttf" };

	inline constexpr std::array kTextFonts = { kRegularFont, kBoldFont, kMonospaceFont };

	inline constexpr std::array kFallbackFonts = {
		FontFile{ "emoji", "rsc/Emoji.ttf" },
		FontFile{ "symbols", "rsc/FontSymbols.ttf" },
		FontFile{ "math", "rsc/FontMath.ttf" },
		FontFile{ "specials", "rsc/FontSpecials.ttf" },
	};
}
