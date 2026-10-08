#pragma once

#include "Color.hpp"
#include "Environment.hpp"
#include "Vec.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace tp::sketch2d {

	enum class BrushType {
		Round,
		Flat,
		Air,
		Pencil,
		Marker,
		Ink,
		Calligraphy,
		Jagged,
		Eraser,
		Smudge,
	};

	enum class BrushParameter {
		Size,
		Opacity,
		Flow,
		Texture,
		Dampen,
		Jagged,
		Interior,
		Hardness,
		Roundness,
		Angle,
		Spacing,
		GrainScale,
		DeckleScale,
		Resaturation,
		Smudge,
		Blur,
		MinSize,
		MinOpacity,
		MinFlow,
		MinBristle,
		MinInterior,
		FirmSize,
		FirmOpacity,
		FirmFlow,
		FirmBristle,
		FirmInterior,
		Count,
	};

	constexpr size_t kBrushParameterCount = (size_t) BrushParameter::Count;

	struct BrushParameterInfo {
		const char* name;
		halnf minimum;
		halnf maximum;
		halnf fallback;
	};

	[[nodiscard]] const BrushParameterInfo& brushParameterInfo(BrushParameter parameter);

	enum class PressureTarget { Size, Opacity, Flow, Bristle, Interior, Count };

	constexpr size_t kPressureTargetCount = (size_t) PressureTarget::Count;

	[[nodiscard]] const char* pressureTargetName(PressureTarget target);

	struct DabShape {
		halnf size = 0;
		halnf opacity = 0;
		halnf flow = 0;
		halnf bristle = 0;
		halnf interior = 0;
		halnf hardness = 1;
		halnf roundness = 1;
		halnf angleDegrees = 0;
		halnf texture = 0;
		halnf grainScale = 2;
		halnf jagged = 0;
		halnf deckleScale = 4;

		[[nodiscard]] halnf radius() const { return size * 0.5f; }
		[[nodiscard]] halnf coverage(halnf x, halnf y, halnf* distanceOut = nullptr) const;
		[[nodiscard]] halnf textured(halnf coverage, Vec2F world, halnf distance) const;
	};

	[[nodiscard]] halnf tipFalloff(halnf distance, halnf hardness);

	class PreparedDab {
	public:
		static constexpr int4 kFalloffSteps = 1024;

		PreparedDab() = default;
		explicit PreparedDab(const DabShape& dab);

		[[nodiscard]] const DabShape& shape() const { return mShape; }
		[[nodiscard]] bool isEmpty() const { return mEmpty; }
		[[nodiscard]] bool plain() const { return mShape.texture <= 0.0f && mShape.jagged <= 0.0f; }

		[[nodiscard]] bool rowSpan(halnf y, halnf& from, halnf& to) const;

		[[nodiscard]] halnf coverage(halnf x, halnf y, halnf& distance) const {
			const halnf along = (x * mCosine + y * mSine) * mInverseAlong;
			const halnf across = (y * mCosine - x * mSine) * mInverseAcross;
			const halnf squared = along * along + across * across;
			if (squared >= 1.0f) return 0;
			distance = std::sqrt(squared);
			const halnf falloff = distance <= mCore ? 1.0f : ringFalloff(distance);
			return falloff * (mInterior + (1.0f - mInterior) * distance);
		}

	private:
		[[nodiscard]] halnf ringFalloff(halnf distance) const;

		DabShape mShape;
		halnf mCosine = 1;
		halnf mSine = 0;
		halnf mInverseAlong = 0;
		halnf mInverseAcross = 0;
		halnf mCore = 1;
		halnf mRingScale = 0;
		halnf mInterior = 1;
		halnf mSpanSquare = 0;
		halnf mSpanCross = 0;
		halnf mSpanRow = 0;
		bool mEmpty = true;
	};

	class PaperTexture {
	public:
		static constexpr int4 kSize = 256;

		explicit PaperTexture(uint32_t seed);

		[[nodiscard]] halnf at(int4 x, int4 y) const {
			return mValues[(size_t) ((y & (kSize - 1)) * kSize + (x & (kSize - 1)))];
		}
		[[nodiscard]] halnf sample(halnf x, halnf y) const;
		[[nodiscard]] halnf mean() const;

		[[nodiscard]] static const PaperTexture& grain();
		[[nodiscard]] static const PaperTexture& deckle();

	private:
		std::vector<halnf> mValues;
	};

	struct BrushPreset;

	class Brush {
	public:
		static constexpr halnf kDefaultStabilizer = 0.325f;
		static constexpr halnf kMaximumSize = 10000;

		Brush();

		BrushType type = BrushType::Round;
		RGBA color = { 0.8f, 0.8f, 0.8f, 1.0f };

		[[nodiscard]] halnf get(BrushParameter parameter) const { return mValues[(size_t) parameter]; }
		void set(BrushParameter parameter, halnf value);

		[[nodiscard]] bool usesPressure(PressureTarget target) const { return mPressure[(size_t) target]; }
		void setUsesPressure(PressureTarget target, bool uses) { mPressure[(size_t) target] = uses; }

		[[nodiscard]] bool markerBlend() const { return mMarkerBlend; }
		void setMarkerBlend(bool marker) { mMarkerBlend = marker; }

		[[nodiscard]] bool linearMixing() const { return mLinearMixing; }
		void setLinearMixing(bool linear) { mLinearMixing = linear; }

		[[nodiscard]] bool smudges() const { return type == BrushType::Smudge; }

		[[nodiscard]] bool eraserMode() const { return mEraserMode; }
		void setEraserMode(bool erase) { mEraserMode = erase; }
		[[nodiscard]] bool erases() const { return type == BrushType::Eraser || (mEraserMode && !smudges()); }

		[[nodiscard]] halnf stabilizer() const { return mStabilizer; }
		void setStabilizer(halnf amount);

		[[nodiscard]] halnf minimumSize() const { return mMinimumSize; }
		[[nodiscard]] halnf maximumSize() const { return mMaximumSize; }
		void setSizeLimits(halnf minimum, halnf maximum);

		[[nodiscard]] halnf size() const { return get(BrushParameter::Size); }
		[[nodiscard]] halnf radius() const { return size() * 0.5f; }
		[[nodiscard]] halnf opacity() const { return get(BrushParameter::Opacity); }
		[[nodiscard]] halnf flow() const { return get(BrushParameter::Flow); }
		[[nodiscard]] halnf hardness() const { return get(BrushParameter::Hardness); }
		[[nodiscard]] halnf spacing() const { return get(BrushParameter::Spacing); }
		void setSize(halnf size) { set(BrushParameter::Size, size); }
		void setOpacity(halnf opacity) { set(BrushParameter::Opacity, opacity); }
		void setFlow(halnf flow) { set(BrushParameter::Flow, flow); }
		void setHardness(halnf hardness) { set(BrushParameter::Hardness, hardness); }
		void setSpacing(halnf spacing) { set(BrushParameter::Spacing, spacing); }

		[[nodiscard]] halnf dampedPressure(halnf pressure) const;
		[[nodiscard]] halnf pressureScale(PressureTarget target, halnf pressure) const;
		[[nodiscard]] DabShape dabAt(halnf pressure) const;
		[[nodiscard]] halnf dabDistance(halnf pressure) const;

		void apply(const BrushPreset& preset);
		[[nodiscard]] const BrushPreset* preset() const;
		void resetEverything();
		void resetParameters();
		void resetStabilization();
		void resetTextures();
		void scaleSize(halnf factor);
		void adjust(BrushParameter parameter, halnf delta);
		[[nodiscard]] const std::string& presetName() const { return mPresetName; }
		void setPresetName(const std::string& name) { mPresetName = name; }

		[[nodiscard]] bool operator==(const Brush& other) const = default;

	private:
		std::array<halnf, kBrushParameterCount> mValues{};
		std::array<bool, kPressureTargetCount> mPressure{};
		halnf mStabilizer = kDefaultStabilizer;
		bool mMarkerBlend = false;
		bool mLinearMixing = false;
		bool mEraserMode = false;
		halnf mMinimumSize = 1;
		halnf mMaximumSize = kMaximumSize;
		std::string mPresetName;
	};

	struct BrushPresetValue {
		BrushParameter parameter;
		halnf value;
	};

	struct BrushPreset {
		const char* name;
		const char* path;
		const char* guid;
		BrushType type;
		uint32_t pressureFlags;
		std::vector<BrushPresetValue> values;
		halnf maximumSize = Brush::kMaximumSize;
		bool markerBlend = false;
	};

	namespace PressureFlag {
		constexpr uint32_t kStabilizer = 0x800;
		constexpr uint32_t kSize = 0x1000;
		constexpr uint32_t kFlow = 0x2000;
		constexpr uint32_t kOpacity = 0x4000;
		constexpr uint32_t kBristle = 0x8000;
		constexpr uint32_t kInterior = 0x10000;
		constexpr uint32_t kLinearMixing = 0x40000;
	}

	[[nodiscard]] const std::vector<BrushPreset>& brushPresets();
	[[nodiscard]] const BrushPreset* findBrushPreset(BrushType type);
}
