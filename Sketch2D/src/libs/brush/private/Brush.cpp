#include "Brush.hpp"

#include <algorithm>
#include <cmath>

#include "Noise.hpp"

namespace tp::sketch2d {

	namespace {
		constexpr halnf kMinimumDabDistance = 0.25f;
		constexpr halnf kPi = 3.14159265358979f;
		constexpr halnf kMaximumHardness = 1.0f - 1.0f / 256.0f;

		const BrushParameterInfo kParameters[kBrushParameterCount] = {
			{ "size", 1, Brush::kMaximumSize, 10 },
			{ "opacity", 0.01f, 1, 1 },
			{ "flow", 0.01f, 1, 1 },
			{ "texture", 0, 1, 0 },
			{ "dampen", 1, 4, 1 },
			{ "jagged", 0, 1, 1 },
			{ "interior", 0, 1, 1 },
			{ "hardness", 0, 1, 0.8f },
			{ "roundness", 0.1f, 1, 1 },
			{ "angle", 0, 360, 0 },
			{ "spacing", 0.01f, 2, 0.1f },
			{ "grain_scale", 1, 5, 2 },
			{ "deckle_scale", 1, 10, 4 },
			{ "resaturation", 0, 0.01f, 0 },
			{ "smudge", 0, 0.99f, 0 },
			{ "blur", 0, 6, 1 },
			{ "min_size", 0, 1, 0 },
			{ "min_opacity", 0, 1, 0 },
			{ "min_flow", 0, 1, 0 },
			{ "min_bristle", 0, 1, 0 },
			{ "min_interior", 0, 1, 0 },
			{ "firm_size", 0.1f, 4, 1 },
			{ "firm_opacity", 0.1f, 4, 1 },
			{ "firm_flow", 0.1f, 4, 1 },
			{ "firm_bristle", 0.1f, 4, 1 },
			{ "firm_interior", 0.1f, 4, 1 },
		};

		const char* const kPressureTargetNames[kPressureTargetCount] = {
			"pressure_size", "pressure_opacity", "pressure_flow", "pressure_bristle", "pressure_interior",
		};

		struct PressureCurve {
			BrushParameter minimum;
			BrushParameter firmness;
		};

		const PressureCurve kCurves[kPressureTargetCount] = {
			{ BrushParameter::MinSize, BrushParameter::FirmSize },
			{ BrushParameter::MinOpacity, BrushParameter::FirmOpacity },
			{ BrushParameter::MinFlow, BrushParameter::FirmFlow },
			{ BrushParameter::MinBristle, BrushParameter::FirmBristle },
			{ BrushParameter::MinInterior, BrushParameter::FirmInterior },
		};
	}

	halnf tipFalloff(halnf distance, halnf hardness) {
		if (distance >= 1.0f) return 0;
		const halnf core = std::min(hardness, kMaximumHardness);
		if (distance <= core) return 1;
		const halnf along = (distance - core) / (1.0f - core);
		return (1.0f + std::cos(kPi * along)) * 0.5f;
	}

	namespace {
		struct FalloffTable {
			std::array<halnf, PreparedDab::kFalloffSteps + 2> values{};

			FalloffTable() {
				for (int4 index = 0; index <= PreparedDab::kFalloffSteps; index++) {
					const halnf along = (halnf) index / (halnf) PreparedDab::kFalloffSteps;
					values[(size_t) index] = (1.0f + std::cos(kPi * along)) * 0.5f;
				}
				values[PreparedDab::kFalloffSteps + 1] = 0;
			}
		};

		const FalloffTable& falloffTable() {
			static const FalloffTable table;
			return table;
		}
	}

	PreparedDab::PreparedDab(const DabShape& dab) :
		mShape(dab) {
		const halnf radius = dab.radius();
		if (radius <= 0.0f || dab.roundness <= 0.0f) return;

		const halnf radians = dab.angleDegrees * kPi / 180.0f;
		mCosine = std::cos(radians);
		mSine = std::sin(radians);
		mInverseAlong = 1.0f / radius;
		mInverseAcross = 1.0f / (radius * dab.roundness);
		mCore = std::min(dab.hardness, kMaximumHardness);
		mRingScale = (halnf) kFalloffSteps / (1.0f - mCore);
		mInterior = dab.interior;

		const halnf along = mInverseAlong * mInverseAlong;
		const halnf across = mInverseAcross * mInverseAcross;
		mSpanSquare = along * mCosine * mCosine + across * mSine * mSine;
		mSpanCross = 2.0f * mCosine * mSine * (along - across);
		mSpanRow = along * mSine * mSine + across * mCosine * mCosine;
		mEmpty = false;
		(void) falloffTable();
	}

	bool PreparedDab::rowSpan(halnf y, halnf& from, halnf& to) const {
		if (mEmpty) return false;
		const double a = mSpanSquare;
		const double b = (double) mSpanCross * y;
		const double c = (double) mSpanRow * y * y - 1.0;
		const double discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) return false;
		const double root = std::sqrt(discriminant);
		from = (halnf) ((-b - root) / (2.0 * a));
		to = (halnf) ((-b + root) / (2.0 * a));
		return true;
	}

	halnf PreparedDab::ringFalloff(halnf distance) const {
		const halnf position = (distance - mCore) * mRingScale;
		const int4 index = (int4) position;
		if (index >= kFalloffSteps) return 0;
		const halnf* values = falloffTable().values.data();
		const halnf low = values[index];
		return low + (values[index + 1] - low) * (position - (halnf) index);
	}

	PaperTexture::PaperTexture(uint32_t seed) :
		mValues((size_t) kSize * kSize) {
		const TileableNoise noise(seed);
		const halnf weights[] = { 1, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };
		constexpr int4 kBasePeriod = 8;
		for (int4 y = 0; y < kSize; y++) {
			for (int4 x = 0; x < kSize; x++) {
				const halnf u = (halnf) x * kBasePeriod / kSize;
				const halnf v = (halnf) y * kBasePeriod / kSize;
				const halnf value = noise.fractal(u, v, kBasePeriod, weights, 6);
				mValues[(size_t) (y * kSize + x)] = value;
			}
		}
		const auto [low, high] = std::minmax_element(mValues.begin(), mValues.end());
		const halnf lowest = *low;
		const halnf range = *high - *low;
		for (halnf& value : mValues)
			value = range > 0 ? (value - lowest) / range : 0.5f;
	}

	halnf PaperTexture::sample(halnf x, halnf y) const {
		const halnf fx = std::floor(x);
		const halnf fy = std::floor(y);
		const int4 ix = (int4) fx;
		const int4 iy = (int4) fy;
		const halnf tx = x - fx;
		const halnf ty = y - fy;
		const halnf top = at(ix, iy) + (at(ix + 1, iy) - at(ix, iy)) * tx;
		const halnf bottom = at(ix, iy + 1) + (at(ix + 1, iy + 1) - at(ix, iy + 1)) * tx;
		return top + (bottom - top) * ty;
	}

	halnf PaperTexture::mean() const {
		halnf sum = 0;
		for (halnf value : mValues)
			sum += value;
		return sum / (halnf) mValues.size();
	}

	const PaperTexture& PaperTexture::grain() {
		static const PaperTexture texture(0x4752414e);
		return texture;
	}

	const PaperTexture& PaperTexture::deckle() {
		static const PaperTexture texture(0x4445434b);
		return texture;
	}

	halnf DabShape::textured(halnf value, Vec2F world, halnf distance) const {
		if (value <= 0.0f) return 0;
		if (jagged > 0.0f && distance > 0.0f) {
			const halnf edge = PaperTexture::deckle().sample(world.x / deckleScale, world.y / deckleScale);
			const halnf threshold = 1.0f - jagged * (1.0f - edge) * distance;
			value *= std::clamp((threshold - distance * distance) * 8.0f + 0.5f, 0.0f, 1.0f);
		}
		if (texture > 0.0f) {
			const halnf paper = PaperTexture::grain().sample(world.x / grainScale, world.y / grainScale);
			const halnf tooth = std::clamp((paper - (1.0f - value) * 0.5f) * 2.0f, 0.0f, 1.0f);
			value *= 1.0f - texture + texture * tooth;
		}
		return std::clamp(value, 0.0f, 1.0f);
	}

	halnf DabShape::coverage(halnf x, halnf y, halnf* distanceOut) const {
		const halnf r = radius();
		if (r <= 0.0f) return 0;
		const halnf radians = angleDegrees * kPi / 180.0f;
		const halnf cosine = std::cos(radians);
		const halnf sine = std::sin(radians);
		const halnf along = (x * cosine + y * sine) / r;
		const halnf across = (y * cosine - x * sine) / (r * roundness);
		const halnf distance = std::sqrt(along * along + across * across);
		if (distanceOut) *distanceOut = distance;
		const halnf falloff = tipFalloff(distance, hardness);
		if (falloff <= 0.0f) return 0;
		return falloff * (interior + (1.0f - interior) * distance);
	}

	const BrushParameterInfo& brushParameterInfo(BrushParameter parameter) { return kParameters[(size_t) parameter]; }

	const char* pressureTargetName(PressureTarget target) { return kPressureTargetNames[(size_t) target]; }

	Brush::Brush() {
		for (size_t index = 0; index < kBrushParameterCount; index++)
			mValues[index] = kParameters[index].fallback;
	}

	void Brush::set(BrushParameter parameter, halnf value) {
		const BrushParameterInfo& info = brushParameterInfo(parameter);
		halnf minimum = info.minimum;
		halnf maximum = info.maximum;
		if (parameter == BrushParameter::Size) {
			minimum = mMinimumSize;
			maximum = mMaximumSize;
		}
		mValues[(size_t) parameter] = std::clamp(value, minimum, maximum);
	}

	void Brush::setStabilizer(halnf amount) { mStabilizer = std::clamp(amount, 0.0f, 1.0f); }

	void Brush::setSizeLimits(halnf minimum, halnf maximum) {
		const BrushParameterInfo& info = brushParameterInfo(BrushParameter::Size);
		mMinimumSize = std::clamp(minimum, info.minimum, info.maximum);
		mMaximumSize = std::clamp(maximum, mMinimumSize, info.maximum);
		set(BrushParameter::Size, size());
	}

	halnf Brush::dampedPressure(halnf pressure) const {
		const halnf clamped = std::clamp(pressure, 0.0f, 1.0f);
		const halnf dampen = get(BrushParameter::Dampen);
		return dampen == 1.0f ? clamped : std::pow(clamped, dampen);
	}

	halnf Brush::pressureScale(PressureTarget target, halnf pressure) const {
		if (!usesPressure(target)) return 1;

		const PressureCurve& curve = kCurves[(size_t) target];
		const halnf minimum = get(curve.minimum);
		const halnf firmness = get(curve.firmness);
		const halnf damped = dampedPressure(pressure);
		return minimum + (1.0f - minimum) * std::pow(damped, firmness);
	}

	DabShape Brush::dabAt(halnf pressure) const {
		DabShape dab;
		dab.size = size() * pressureScale(PressureTarget::Size, pressure);
		dab.opacity = opacity() * pressureScale(PressureTarget::Opacity, pressure);
		dab.flow = flow() * pressureScale(PressureTarget::Flow, pressure);
		dab.bristle = pressureScale(PressureTarget::Bristle, pressure);
		dab.interior = get(BrushParameter::Interior) * pressureScale(PressureTarget::Interior, pressure);
		dab.hardness = hardness();
		dab.roundness = get(BrushParameter::Roundness);
		dab.angleDegrees = get(BrushParameter::Angle);
		dab.texture = get(BrushParameter::Texture);
		dab.grainScale = get(BrushParameter::GrainScale);
		dab.jagged = type == BrushType::Jagged ? get(BrushParameter::Jagged) : 0.0f;
		dab.deckleScale = get(BrushParameter::DeckleScale);
		return dab;
	}

	halnf Brush::dabDistance(halnf pressure) const {
		const halnf distance = dabAt(pressure).size * spacing();
		return distance > kMinimumDabDistance ? distance : kMinimumDabDistance;
	}
}

namespace tp::sketch2d {

	namespace {
		using P = BrushParameter;
		using namespace PressureFlag;

		std::vector<BrushPreset> makePresets() {
			return {
				{ "Ink",
					"/brush/ink",
					"25F36A7E62CE4BB18B2287F6F5CB0A92",
					BrushType::Ink,
					kSize,
					{ { P::Size, 7 },
						{ P::Spacing, 0.025f },
						{ P::FirmSize, 2.2f },
						{ P::MinSize, 0.15f },
						{ P::FirmOpacity, 1 } } },
				{ "Pencil",
					"/brush/pencil",
					"DE3DE44D1B3E49DFBB194A309A0FDC2F",
					BrushType::Pencil,
					kSize | kFlow,
					{ { P::Size, 2 },
						{ P::Hardness, 0 },
						{ P::Flow, 0.5f },
						{ P::Spacing, 0.15f },
						{ P::FirmSize, 1 },
						{ P::MinSize, 0.7f },
						{ P::FirmFlow, 2.2f } } },
				{ "Marker",
					"/brush/marker",
					"6F4C6CDA177E4A6DABF4078792991930",
					BrushType::Marker,
					0,
					{ { P::Size, 22 },
						{ P::Opacity, 0.5f },
						{ P::Spacing, 0.025f },
						{ P::FirmSize, 2 },
						{ P::MinSize, 0.3f },
						{ P::FirmFlow, 1.75f },
						{ P::FirmOpacity, 1.75f },
						{ P::MinOpacity, 0 } },
					Brush::kMaximumSize,
					true },
				{ "Calligraphy",
					"/brush/calligraphy",
					"96D6DB1B3450420D9E4FF4AE458DCCED",
					BrushType::Calligraphy,
					kSize,
					{ { P::Size, 12 },
						{ P::Angle, 45 },
						{ P::Spacing, 0.05f },
						{ P::Roundness, 0.25f },
						{ P::FirmSize, 2 },
						{ P::MinSize, 0.2f } } },
				{ "Jagged brush",
					"/brush/jagged-brush",
					"E0ECBB60CA26486A97B080E31EC12B93",
					BrushType::Jagged,
					kSize | kFlow | kInterior,
					{ { P::Size, 36 },
						{ P::Spacing, 0.025f },
						{ P::Flow, 0.5f },
						{ P::Texture, 0.8f },
						{ P::Jagged, 0.9f },
						{ P::Interior, 0.8f },
						{ P::FirmSize, 1 },
						{ P::MinSize, 0.5f },
						{ P::FirmFlow, 1.75f },
						{ P::FirmOpacity, 1.75f },
						{ P::MinOpacity, 0 },
						{ P::FirmInterior, 0.8f },
						{ P::MinInterior, 0.3f } } },
				{ "Round brush",
					"/brush/round-brush",
					"529AB1FF4AB7462C8F0106663715356B",
					BrushType::Round,
					kSize | kFlow,
					{ { P::Size, 20 },
						{ P::Spacing, 0.025f },
						{ P::FirmSize, 1.8f },
						{ P::MinSize, 0.25f },
						{ P::FirmFlow, 1.8f },
						{ P::FirmOpacity, 1.8f },
						{ P::MinOpacity, 0 } } },
				{ "Flat brush",
					"/brush/flat-brush",
					"630B01DF64EB4683A957DD62D47B90CD",
					BrushType::Flat,
					kFlow,
					{ { P::Size, 24 },
						{ P::Spacing, 0.025f },
						{ P::Texture, 0.8f },
						{ P::Angle, 45 },
						{ P::FirmSize, 1 },
						{ P::MinSize, 0.25f },
						{ P::FirmFlow, 1.75f },
						{ P::FirmOpacity, 1.75f },
						{ P::MinOpacity, 0 } } },
				{ "Air brush",
					"/brush/air-brush",
					"6EFCF4E46C0640079055DDFF658D9A84",
					BrushType::Air,
					kFlow | kOpacity,
					{ { P::Size, 48 },
						{ P::Spacing, 0.05f },
						{ P::Hardness, 0 },
						{ P::Flow, 0.25f },
						{ P::FirmSize, 1.75f },
						{ P::MinSize, 0 },
						{ P::FirmFlow, 1 } } },
				{ "Eraser",
					"/brush-eraser/",
					"8C85D19797EF4C059B2CDFC3DCAC1A0B",
					BrushType::Eraser,
					kFlow,
					{ { P::Size, 32 },
						{ P::Spacing, 0.05f },
						{ P::FirmSize, 1 },
						{ P::MinSize, 0.25f },
						{ P::FirmFlow, 1 },
						{ P::FirmOpacity, 1.75f },
						{ P::MinOpacity, 0 } } },
				{ "Smudge",
					"/brush-smudge/",
					"ECCD1B016E8F4CD189B0E6DD44107FBF",
					BrushType::Smudge,
					kFlow,
					{ { P::Size, 40 },
						{ P::Hardness, 0.5f },
						{ P::Smudge, 0.8f },
						{ P::Flow, 0.1f },
						{ P::Spacing, 0.01f },
						{ P::FirmFlow, 1.75f },
						{ P::MinFlow, 0 } },
					200 },
			};
		}
	}

	const std::vector<BrushPreset>& brushPresets() {
		static const std::vector<BrushPreset> presets = makePresets();
		return presets;
	}

	namespace {
		constexpr BrushParameter kTextureParameters[] = {
			BrushParameter::Texture, BrushParameter::GrainScale, BrushParameter::DeckleScale, BrushParameter::Jagged
		};

	}

	const BrushPreset* Brush::preset() const {
		for (const BrushPreset& candidate : brushPresets())
			if (!mPresetName.empty() && mPresetName == candidate.name && candidate.type == type) return &candidate;
		return findBrushPreset(type);
	}

	void Brush::resetEverything() {
		if (const BrushPreset* original = preset()) apply(*original);
	}

	void Brush::resetParameters() {
		const BrushPreset* original = preset();
		if (!original) return;
		Brush fresh;
		fresh.color = color;
		fresh.apply(*original);
		fresh.mStabilizer = mStabilizer;
		fresh.mEraserMode = mEraserMode;
		for (const BrushParameter texture : kTextureParameters)
			fresh.set(texture, get(texture));
		*this = fresh;
	}

	void Brush::resetStabilization() { mStabilizer = kDefaultStabilizer; }

	void Brush::resetTextures() {
		const BrushPreset* original = preset();
		if (!original) return;
		Brush fresh;
		fresh.apply(*original);
		for (const BrushParameter texture : kTextureParameters)
			set(texture, fresh.get(texture));
	}

	void Brush::scaleSize(halnf factor) {
		const halnf current = size();
		halnf next = current * factor;
		if (factor > 1 && next < current + 1) next = current + 1;
		if (factor < 1 && next > current - 1) next = current - 1;
		setSize(std::round(next));
	}

	void Brush::adjust(BrushParameter parameter, halnf delta) {
		if (parameter == BrushParameter::Angle) {
			const halnf angle = std::fmod(get(parameter) + delta, 360.0f);
			set(parameter, angle < 0 ? angle + 360.0f : angle);
			return;
		}
		set(parameter, get(parameter) + delta);
	}

	const BrushPreset* findBrushPreset(BrushType type) {
		for (const BrushPreset& preset : brushPresets()) {
			if (preset.type == type) return &preset;
		}
		return nullptr;
	}

	void Brush::apply(const BrushPreset& preset) {
		const RGBA keptColor = color;
		*this = Brush();
		color = keptColor;
		type = preset.type;
		mPresetName = preset.name;
		setSizeLimits(1, preset.maximumSize);
		setMarkerBlend(preset.markerBlend);
		setLinearMixing(preset.pressureFlags & PressureFlag::kLinearMixing);
		for (const BrushPresetValue& value : preset.values)
			set(value.parameter, value.value);

		setUsesPressure(PressureTarget::Size, preset.pressureFlags & PressureFlag::kSize);
		setUsesPressure(PressureTarget::Flow, preset.pressureFlags & PressureFlag::kFlow);
		setUsesPressure(PressureTarget::Opacity, preset.pressureFlags & PressureFlag::kOpacity);
		setUsesPressure(PressureTarget::Bristle, preset.pressureFlags & PressureFlag::kBristle);
		setUsesPressure(PressureTarget::Interior, preset.pressureFlags & PressureFlag::kInterior);
	}
}
