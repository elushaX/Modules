#include "ColorModel.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

namespace tp::sketch2d {

	namespace {
		halnf wrapHue(halnf degrees) { return std::fmod(std::fmod(degrees, 360.0f) + 360.0f, 360.0f); }

		int4 channelByte(halnf channel) { return (int4) std::lround(std::clamp(channel, 0.0f, 1.0f) * 255.0f); }

		int4 hexDigit(char digit) {
			if (digit >= '0' && digit <= '9') return digit - '0';
			const char lower = (char) std::tolower((unsigned char) digit);
			if (lower >= 'a' && lower <= 'f') return lower - 'a' + 10;
			return -1;
		}
	}

	const HueColor ColorPair::kDefaultPrimary = { 220.0f, 0.6f, 0.5f };
	const HueColor ColorPair::kDefaultSecondary = { 0.0f, 0.0f, 1.0f };

	HueColor hueColorOf(const RGBA& color) {
		const halnf high = std::max({ color.r, color.g, color.b });
		const halnf low = std::min({ color.r, color.g, color.b });
		const halnf chroma = high - low;
		HueColor hue{ 0, 0, high };
		if (high <= 0 || chroma <= 1e-6f) return hue;
		hue.saturation = chroma / high;
		halnf sector = 0;
		if (high == color.r) sector = std::fmod((color.g - color.b) / chroma, 6.0f);
		else if (high == color.g) sector = (color.b - color.r) / chroma + 2.0f;
		else sector = (color.r - color.g) / chroma + 4.0f;
		hue.hueDegrees = wrapHue(sector * 60.0f);
		return hue;
	}

	HslColor hslOf(const HueColor& color) {
		const halnf lightness = color.value * (1 - color.saturation / 2);
		const halnf denominator = std::min(lightness, 1 - lightness);
		const halnf saturation = denominator <= 1e-6f ? 0 : (color.value - lightness) / denominator;
		return { color.hueDegrees, std::clamp(saturation, 0.0f, 1.0f), lightness };
	}

	HueColor hueColorOf(const HslColor& color) {
		const halnf value = color.lightness + color.saturation * std::min(color.lightness, 1 - color.lightness);
		const halnf saturation = value <= 1e-6f ? 0 : 2 * (1 - color.lightness / value);
		return { color.hueDegrees, std::clamp(saturation, 0.0f, 1.0f), std::clamp(value, 0.0f, 1.0f) };
	}

	std::string colorHex(const RGBA& color) {
		char text[8];
		std::snprintf(text, sizeof(text), "%02X%02X%02X", channelByte(color.r), channelByte(color.g), channelByte(color.b));
		return text;
	}

	std::optional<RGBA> parseColorHex(const std::string& text) {
		std::string digits;
		for (const char c : text)
			if (!std::isspace((unsigned char) c) && c != '#') digits += c;
		if (digits.size() == 3) digits = { digits[0], digits[0], digits[1], digits[1], digits[2], digits[2] };
		if (digits.size() != 6) return std::nullopt;
		int4 bytes[3];
		for (int4 index = 0; index < 3; index++) {
			const int4 high = hexDigit(digits[(size_t) index * 2]);
			const int4 low = hexDigit(digits[(size_t) index * 2 + 1]);
			if (high < 0 || low < 0) return std::nullopt;
			bytes[index] = high * 16 + low;
		}
		return RGBA((halnf) bytes[0] / 255.0f, (halnf) bytes[1] / 255.0f, (halnf) bytes[2] / 255.0f, 1.0f);
	}

	void ColorPair::setPrimary(const RGBA& color) {
		const HueColor converted = hueColorOf(color);
		const bool gray = converted.saturation <= 1e-6f || converted.value <= 1e-6f;
		mPrimary = { gray ? mPrimary.hueDegrees : converted.hueDegrees, converted.saturation, converted.value };
	}

	void ColorPair::resetToDefaults() {
		mPrimary.value = 0.0f;
		mSecondary.value = 1.0f;
	}

	void ColorPair::swap() { std::swap(mPrimary, mSecondary); }

	void ColorPair::cooler(halnf degrees) {
		const halnf hue = wrapHue(mPrimary.hueDegrees);
		if (hue < kCoolHue) mPrimary.hueDegrees = std::min(hue + degrees, kCoolHue);
		else mPrimary.hueDegrees = std::max(hue - degrees, kCoolHue);
	}

	void ColorPair::warmer(halnf degrees) {
		const halnf hue = wrapHue(mPrimary.hueDegrees);
		if (hue <= kCoolHue) mPrimary.hueDegrees = std::max(hue - degrees, 0.0f);
		else mPrimary.hueDegrees = wrapHue(std::min(hue + degrees, 360.0f));
	}

	void ColorPair::complement() { mPrimary.hueDegrees = wrapHue(mPrimary.hueDegrees + 180.0f); }

	void ColorPair::rotateHue(halnf degrees) { mPrimary.hueDegrees = wrapHue(mPrimary.hueDegrees + degrees); }

	void ColorPair::changeSaturation(halnf delta) {
		mPrimary.saturation = std::clamp(mPrimary.saturation + delta, 0.0f, 1.0f);
	}

	void ColorPair::changeValue(halnf delta) { setValue(mPrimary.value + delta); }

	void ColorPair::setValue(halnf value) { mPrimary.value = std::clamp(value, 0.0f, 1.0f); }

	namespace {
		bool sameHex(const RGBA& left, const RGBA& right) { return colorHex(left) == colorHex(right); }
	}

	void ColorHistory::recordStroke(const HueColor& color) {
		const RGBA rgb = color.rgba();
		int4 strokes = 0;
		for (size_t index = 0; index < mEntries.size(); index++) {
			if (!sameHex(mEntries[index].color.rgba(), rgb)) continue;
			strokes = mEntries[index].strokes;
			mEntries.erase(mEntries.begin() + (std::ptrdiff_t) index);
			break;
		}
		mEntries.insert(mEntries.begin(), { color, strokes + 1 });
		if (mEntries.size() > kCapacity) mEntries.resize(kCapacity);
	}

	void ColorHistory::restore(std::vector<Entry> entries) {
		mEntries = std::move(entries);
		if (mEntries.size() > kCapacity) mEntries.resize(kCapacity);
	}

	bool SwatchSet::add(const RGBA& color) {
		for (const RGBA& existing : mColors)
			if (sameHex(existing, color)) return false;
		if (mColors.size() >= kCapacity) return false;
		mColors.push_back(RGBA(color.r, color.g, color.b, 1));
		return true;
	}

	void SwatchSet::remove(size_t index) {
		if (index < mColors.size()) mColors.erase(mColors.begin() + (std::ptrdiff_t) index);
	}

	std::string SwatchSet::toJson(int8 created, int8 modified) const {
		std::string text = "{\n\t\"object\" : \"swatches\",\n";
		text += "\t\"created\" : \"" + std::to_string(created) + "\",\n";
		text += "\t\"modified\" : \"" + std::to_string(modified) + "\",\n";
		text += "\t\"data\" : [\n";
		for (size_t index = 0; index < mColors.size(); index++) {
			text += "\t\t{ \"hex\" : \"#" + colorHex(mColors[index]) + "\" }";
			text += index + 1 < mColors.size() ? ",\n" : "\n";
		}
		return text + "\t]\n}\n";
	}

	std::optional<SwatchSet> SwatchSet::fromJson(const std::string& text) {
		const size_t object = text.find("\"object\"");
		if (object == std::string::npos) return std::nullopt;
		const size_t kind = text.find('"', text.find(':', object));
		if (kind == std::string::npos || text.compare(kind, 10, "\"swatches\"") != 0) return std::nullopt;
		const size_t data = text.find("\"data\"");
		if (data == std::string::npos) return std::nullopt;
		SwatchSet set;
		size_t cursor = text.find('[', data);
		if (cursor == std::string::npos) return std::nullopt;
		const size_t end = text.find(']', cursor);
		while (true) {
			const size_t hex = text.find("\"hex\"", cursor);
			if (hex == std::string::npos || hex > end) break;
			const size_t open = text.find('"', text.find(':', hex));
			const size_t close = open == std::string::npos ? open : text.find('"', open + 1);
			if (close == std::string::npos) return std::nullopt;
			const auto parsed = parseColorHex(text.substr(open + 1, close - open - 1));
			if (!parsed) return std::nullopt;
			set.add(*parsed);
			cursor = close + 1;
		}
		return set;
	}

	bool SwatchSet::save(const std::string& path) const {
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file) return false;
		const int8 now = (int8) std::time(nullptr);
		file << toJson(now, now);
		return (bool) file;
	}

	SwatchSet SwatchSet::load(const std::string& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file) return {};
		std::stringstream text;
		text << file.rdbuf();
		return fromJson(text.str()).value_or(SwatchSet());
	}
}
