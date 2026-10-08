#include "CanvasFonts.hpp"

#include "stb_truetype.h"

#include "UnitTest++/UnitTest++.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace {
	const std::filesystem::path kSourceRoot = MODULES_SOURCE_DIR;
	const std::filesystem::path kResourceRoot = kSourceRoot / "Graphics";
	const std::vector<std::string> kUiSourceDirectories = {
		"Widgets/private", "Widgets/public", "Taskboard/private", "Taskboard/public"
	};

	class LoadedFont {
	public:
		explicit LoadedFont(const tp::FontFile& file) {
			std::ifstream stream(kResourceRoot / file.path, std::ios::binary);
			mBytes.assign(std::istreambuf_iterator<char>(stream), {});
			const auto* data = reinterpret_cast<const unsigned char*>(mBytes.data());
			mValid = !mBytes.empty() && stbtt_InitFont(&mInfo, data, stbtt_GetFontOffsetForIndex(data, 0));
		}

		[[nodiscard]] bool isValid() const { return mValid; }
		[[nodiscard]] bool hasGlyph(char32_t codepoint) const {
			return mValid && stbtt_FindGlyphIndex(&mInfo, int(codepoint)) != 0;
		}

	private:
		std::vector<char> mBytes;
		stbtt_fontinfo mInfo{};
		bool mValid = false;
	};

	std::optional<char32_t> decodeUtf8(const std::string& text, size_t& position) {
		const auto byte = [&](size_t offset) { return static_cast<unsigned char>(text[position + offset]); };
		const unsigned char lead = byte(0);
		const size_t length = lead < 0x80 ? 1 : lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 0;
		if (length == 0 || position + length > text.size()) {
			position++;
			return std::nullopt;
		}

		char32_t codepoint = length == 1 ? lead : lead & (0xFF >> (length + 1));
		for (size_t index = 1; index < length; index++)
			codepoint = (codepoint << 6) | (byte(index) & 0x3F);
		position += length;
		return codepoint;
	}

	void collectEscapes(const std::string& source, std::set<char32_t>& codepoints) {
		static const std::regex escape(R"(\\u([0-9A-Fa-f]{4})|\\U([0-9A-Fa-f]{8}))");
		for (std::sregex_iterator match(source.begin(), source.end(), escape), end; match != end; ++match) {
			const std::string digits = (*match)[1].matched ? (*match)[1].str() : (*match)[2].str();
			codepoints.insert(char32_t(std::stoul(digits, nullptr, 16)));
		}
	}

	void collectLiterals(const std::string& source, std::set<char32_t>& codepoints) {
		for (size_t position = 0; position < source.size();) {
			const std::optional<char32_t> codepoint = decodeUtf8(source, position);
			if (codepoint && *codepoint >= 0x80) codepoints.insert(*codepoint);
		}
	}

	std::set<char32_t> codepointsUsedByUi() {
		std::set<char32_t> codepoints;
		for (const std::string& directory : kUiSourceDirectories) {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(kSourceRoot / directory)) {
				if (!entry.is_regular_file()) continue;
				std::ifstream stream(entry.path(), std::ios::binary);
				const std::string source{ std::istreambuf_iterator<char>(stream), {} };
				collectEscapes(source, codepoints);
				collectLiterals(source, codepoints);
			}
		}
		return codepoints;
	}

	std::string hex(char32_t codepoint) {
		char buffer[16];
		std::snprintf(buffer, sizeof(buffer), "U+%04X", unsigned(codepoint));
		return buffer;
	}
}

SUITE(FontCoverage) {
	TEST(EveryBundledFontLoads) {
		for (const tp::FontFile& file : tp::kTextFonts)
			CHECK(LoadedFont(file).isValid());
		for (const tp::FontFile& file : tp::kFallbackFonts)
			CHECK(LoadedFont(file).isValid());
	}

	TEST(EveryCharacterTheUiDrawsHasAGlyphInEveryTextFace) {
		std::vector<LoadedFont> fallbacks;
		for (const tp::FontFile& file : tp::kFallbackFonts)
			fallbacks.emplace_back(file);

		const std::set<char32_t> used = codepointsUsedByUi();
		CHECK(used.count(U'\u2190') == 1);

		for (const tp::FontFile& face : tp::kTextFonts) {
			const LoadedFont primary(face);
			for (const char32_t codepoint : used) {
				bool covered = primary.hasGlyph(codepoint);
				for (const LoadedFont& fallback : fallbacks)
					covered = covered || fallback.hasGlyph(codepoint);
				CHECK_EQUAL(hex(codepoint) + " drawn", hex(codepoint) + (covered ? " drawn" : " has no glyph"));
			}
		}
	}
}
