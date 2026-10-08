#pragma once

#include "Document.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tp::sketch2d {

	struct HslColor {
		halnf hueDegrees = 0;
		halnf saturation = 0;
		halnf lightness = 0;
	};

	[[nodiscard]] HueColor hueColorOf(const RGBA& color);
	[[nodiscard]] HslColor hslOf(const HueColor& color);
	[[nodiscard]] HueColor hueColorOf(const HslColor& color);
	[[nodiscard]] std::string colorHex(const RGBA& color);
	[[nodiscard]] std::optional<RGBA> parseColorHex(const std::string& text);

	class ColorHistory {
	public:
		static constexpr size_t kCapacity = 7;

		struct Entry {
			HueColor color;
			int4 strokes = 0;
		};

		void recordStroke(const HueColor& color);
		void restore(std::vector<Entry> entries);
		void clear() { mEntries.clear(); }
		[[nodiscard]] const std::vector<Entry>& entries() const { return mEntries; }

	private:
		std::vector<Entry> mEntries;
	};

	class SwatchSet {
	public:
		static constexpr size_t kCapacity = 64;

		bool add(const RGBA& color);
		void remove(size_t index);
		[[nodiscard]] const std::vector<RGBA>& colors() const { return mColors; }

		[[nodiscard]] std::string toJson(int8 created, int8 modified) const;
		[[nodiscard]] static std::optional<SwatchSet> fromJson(const std::string& text);
		bool save(const std::string& path) const;
		[[nodiscard]] static SwatchSet load(const std::string& path);

	private:
		std::vector<RGBA> mColors;
	};

	class ColorPair {
	public:
		static constexpr halnf kCoolHue = 240.0f;
		static constexpr halnf kHueStep = 10.0f;
		static constexpr halnf kAmountStep = 0.05f;

		static const HueColor kDefaultPrimary;
		static const HueColor kDefaultSecondary;

		[[nodiscard]] const HueColor& primary() const { return mPrimary; }
		[[nodiscard]] const HueColor& secondary() const { return mSecondary; }
		void setPrimary(const HueColor& color) { mPrimary = color; }
		void setPrimary(const RGBA& color);
		void setSecondary(const HueColor& color) { mSecondary = color; }

		void resetToDefaults();
		void swap();
		void cooler(halnf degrees = kHueStep);
		void warmer(halnf degrees = kHueStep);
		void complement();
		void rotateHue(halnf degrees);
		void changeSaturation(halnf delta);
		void changeValue(halnf delta);
		void setValue(halnf value);

		[[nodiscard]] bool operator==(const ColorPair& other) const = default;

	private:
		HueColor mPrimary = kDefaultPrimary;
		HueColor mSecondary = kDefaultSecondary;
	};
}
