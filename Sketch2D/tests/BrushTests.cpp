#include "Brush.hpp"
#include "Document.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	int4 alphaAt(const Document& document, int4 x, int4 y) {
		document.settlePaint();
		const Tile* tile = document.activeLayer()->tileIfExists({ floordiv(x, kTileSize), floordiv(y, kTileSize) });
		if (!tile) return 0;
		const int4 lx = x - floordiv(x, kTileSize) * kTileSize;
		const int4 ly = y - floordiv(y, kTileSize) * kTileSize;
		return tile->pixels()[((ualni) ly * kTileSize + lx) * 4 + 3];
	}

	int4 paintedWidth(const Document& document, int4 y) {
		int4 width = 0;
		for (int4 x = -200; x < 200; x++)
			width += alphaAt(document, x, y) > 0 ? 1 : 0;
		return width;
	}
}

SUITE(BrushModel) {
	TEST(EveryParameterHasReferenceNameAndRange) {
		const std::string names[] = { "size",         "opacity",     "flow",         "texture",      "dampen",
																	"jagged",       "interior",    "hardness",     "roundness",    "angle",
																	"spacing",      "grain_scale", "deckle_scale", "resaturation", "smudge",
																	"blur",         "min_size",    "min_opacity",  "min_flow",     "min_bristle",
																	"min_interior", "firm_size",   "firm_opacity", "firm_flow",    "firm_bristle",
																	"firm_interior" };
		CHECK_EQUAL(26u, kBrushParameterCount);
		for (size_t index = 0; index < kBrushParameterCount; index++)
			CHECK_EQUAL(names[index], brushParameterInfo((BrushParameter) index).name);

		CHECK_EQUAL(1.0f, brushParameterInfo(BrushParameter::Size).minimum);
		CHECK_EQUAL(10000.0f, brushParameterInfo(BrushParameter::Size).maximum);
		CHECK_EQUAL(360.0f, brushParameterInfo(BrushParameter::Angle).maximum);
		CHECK_CLOSE(0.01f, brushParameterInfo(BrushParameter::Spacing).minimum, 1e-6f);
		CHECK_EQUAL(2.0f, brushParameterInfo(BrushParameter::Spacing).maximum);
		CHECK_CLOSE(0.1f, brushParameterInfo(BrushParameter::FirmSize).minimum, 1e-6f);
		CHECK_EQUAL(4.0f, brushParameterInfo(BrushParameter::FirmInterior).maximum);
	}

	TEST(DefaultsFollowTheOriginalConstructor) {
		const Brush brush;
		CHECK_EQUAL(10.0f, brush.size());
		CHECK_EQUAL(1.0f, brush.opacity());
		CHECK_EQUAL(1.0f, brush.flow());
		CHECK_CLOSE(0.8f, brush.hardness(), 1e-6f);
		CHECK_CLOSE(0.1f, brush.spacing(), 1e-6f);
		CHECK_CLOSE(0.8f, brush.color.r, 1e-6f);
		CHECK_CLOSE(0.8f, brush.color.g, 1e-6f);
		CHECK_CLOSE(0.8f, brush.color.b, 1e-6f);
		CHECK_EQUAL(1.0f, brush.get(BrushParameter::Dampen));
		CHECK_EQUAL(1.0f, brush.get(BrushParameter::FirmSize));
		CHECK_EQUAL(0.0f, brush.get(BrushParameter::MinSize));
		for (size_t index = 0; index < kPressureTargetCount; index++)
			CHECK(!brush.usesPressure((PressureTarget) index));
	}

	TEST(ValuesAreClampedToTheirRange) {
		Brush brush;
		brush.set(BrushParameter::Hardness, 3);
		brush.set(BrushParameter::Spacing, 0);
		brush.setSize(50000);
		CHECK_EQUAL(1.0f, brush.hardness());
		CHECK_CLOSE(0.01f, brush.spacing(), 1e-6f);
		CHECK_EQUAL(10000.0f, brush.size());
	}

	TEST(SizeLimitsNarrowTheSizeRange) {
		Brush brush;
		brush.setSize(500);
		brush.setSizeLimits(1, 200);
		CHECK_EQUAL(200.0f, brush.size());
		brush.setSize(0.5f);
		CHECK_EQUAL(1.0f, brush.size());
	}

	TEST(PressureIgnoredUnlessEnabled) {
		Brush brush;
		brush.setSize(40);
		CHECK_EQUAL(40.0f, brush.dabAt(0.1f).size);
		CHECK_EQUAL(1.0f, brush.dabAt(0.1f).opacity);
	}

	TEST(PressureCurveUsesMinimumAndFirmness) {
		Brush brush;
		brush.setUsesPressure(PressureTarget::Size, true);
		brush.set(BrushParameter::MinSize, 0.2f);
		brush.set(BrushParameter::FirmSize, 2);

		CHECK_CLOSE(0.2f, brush.pressureScale(PressureTarget::Size, 0), 1e-6f);
		CHECK_CLOSE(1.0f, brush.pressureScale(PressureTarget::Size, 1), 1e-6f);
		CHECK_CLOSE(0.2f + 0.8f * 0.25f, brush.pressureScale(PressureTarget::Size, 0.5f), 1e-6f);
		CHECK_EQUAL(1.0f, brush.pressureScale(PressureTarget::Opacity, 0.5f));
	}

	TEST(DampenSoftensLightPressure) {
		Brush brush;
		brush.setUsesPressure(PressureTarget::Flow, true);
		brush.set(BrushParameter::Dampen, 2);
		CHECK_CLOSE(0.25f, brush.pressureScale(PressureTarget::Flow, 0.5f), 1e-6f);
		CHECK_CLOSE(0.25f, brush.dabAt(0.5f).flow, 1e-6f);
	}

	TEST(EveryPressureTargetHasItsOwnCurve) {
		Brush brush;
		for (size_t index = 0; index < kPressureTargetCount; index++)
			brush.setUsesPressure((PressureTarget) index, true);
		brush.set(BrushParameter::MinOpacity, 0.5f);
		brush.set(BrushParameter::MinInterior, 0.25f);

		const DabShape dab = brush.dabAt(0);
		CHECK_EQUAL(0.0f, dab.size);
		CHECK_CLOSE(0.5f, dab.opacity, 1e-6f);
		CHECK_EQUAL(0.0f, dab.flow);
		CHECK_EQUAL(0.0f, dab.bristle);
		CHECK_CLOSE(0.25f, dab.interior, 1e-6f);
		CHECK_EQUAL(std::string("pressure_interior"), pressureTargetName(PressureTarget::Interior));
	}

	TEST(DabDistanceFollowsSpacingAndPressedSize) {
		Brush brush;
		brush.setSize(100);
		brush.setSpacing(0.25f);
		CHECK_CLOSE(25.0f, brush.dabDistance(1), 1e-5f);
		brush.setUsesPressure(PressureTarget::Size, true);
		CHECK_CLOSE(12.5f, brush.dabDistance(0.5f), 1e-5f);
		CHECK_CLOSE(0.25f, brush.dabDistance(0), 1e-6f);
	}

	TEST(PressureChangesThePaintedWidth) {
		Document light;
		Document firm;
		Brush brush;
		brush.setSize(60);
		brush.setHardness(1);
		brush.setUsesPressure(PressureTarget::Size, true);

		light.stampBrushDab({ 0, 0 }, brush, 0.25f);
		firm.stampBrushDab({ 0, 0 }, brush, 1.0f);

		CHECK_CLOSE(15, paintedWidth(light, 0), 2);
		CHECK_CLOSE(60, paintedWidth(firm, 0), 2);
	}

	TEST(PressureChangesThePaintedOpacity) {
		Document document;
		Brush brush;
		brush.setSize(20);
		brush.setUsesPressure(PressureTarget::Opacity, true);
		document.stampBrushDab({ 0, 0 }, brush, 0.5f);
		CHECK_CLOSE(128, alphaAt(document, 0, 0), 2);
	}
}

SUITE(DabShape) {
	DabShape dab(halnf size) {
		DabShape shape;
		shape.size = size;
		shape.opacity = 1;
		shape.flow = 1;
		shape.interior = 1;
		return shape;
	}

	int4 paintedRun(const Document& document, bool horizontal) {
		int4 run = 0;
		for (int4 offset = -200; offset < 200; offset++)
			run += alphaAt(document, horizontal ? offset : 0, horizontal ? 0 : offset) > 0 ? 1 : 0;
		return run;
	}

	TEST(FalloffFollowsReferenceCosineTable) {
		const halnf samples[] = { 16384, 15000, 11318, 6579, 2386, 152 };
		const int indices[] = { 0, 192, 384, 576, 768, 960 };
		for (int index = 0; index < 6; index++) {
			const halnf along = (halnf) indices[index] / 1023.0f;
			CHECK_CLOSE(samples[index] / 16384.0f, tipFalloff(along, 0), 0.002f);
		}
		CHECK_EQUAL(0.0f, tipFalloff(1, 0.5f));
	}

	TEST(HardnessKeepsAFullCore) {
		CHECK_EQUAL(1.0f, tipFalloff(0.5f, 0.6f));
		CHECK(tipFalloff(0.8f, 0.6f) < 1.0f);
		CHECK(tipFalloff(0.8f, 0.6f) > 0.0f);
		CHECK(tipFalloff(0.999f, 1.0f) < 1.0f);
	}

	TEST(RoundnessSquashesAcrossTheAngle) {
		DabShape shape = dab(40);
		shape.roundness = 0.25f;
		CHECK(shape.coverage(15, 0) > 0);
		CHECK_EQUAL(0.0f, shape.coverage(0, 15));
		CHECK(shape.coverage(0, 4) > 0);

		shape.angleDegrees = 90;
		CHECK_EQUAL(0.0f, shape.coverage(15, 0));
		CHECK(shape.coverage(0, 15) > 0);
	}

	TEST(InteriorHollowsTheCentre) {
		DabShape shape = dab(40);
		shape.hardness = 1;
		shape.interior = 0;
		CHECK_CLOSE(0.0f, shape.coverage(0, 0), 1e-5f);
		CHECK(shape.coverage(15, 0) > 0.5f);
		shape.interior = 1;
		CHECK_CLOSE(1.0f, shape.coverage(0, 0), 1e-5f);
	}

	TEST(BrushRoundnessAndAngleReachThePaintedDab) {
		Brush brush;
		brush.setSize(80);
		brush.set(BrushParameter::Roundness, 0.25f);

		Document flat;
		flat.stampBrushDab({ 0, 0 }, brush);
		CHECK_CLOSE(80, paintedRun(flat, true), 3);
		CHECK_CLOSE(20, paintedRun(flat, false), 3);

		brush.set(BrushParameter::Angle, 90);
		Document turned;
		turned.stampBrushDab({ 0, 0 }, brush);
		CHECK_CLOSE(20, paintedRun(turned, true), 3);
		CHECK_CLOSE(80, paintedRun(turned, false), 3);
	}
}

SUITE(BrushPresets) {
	TEST(ReferenceShipsTenBuiltInBrushes) {
		const std::string names[] = { "Ink",         "Pencil",     "Marker",    "Calligraphy", "Jagged brush",
																	"Round brush", "Flat brush", "Air brush", "Eraser",      "Smudge" };
		REQUIRE CHECK_EQUAL(10u, brushPresets().size());
		for (size_t index = 0; index < 10; index++) {
			CHECK_EQUAL(names[index], brushPresets()[index].name);
			CHECK_EQUAL(32u, std::string(brushPresets()[index].guid).size());
		}
	}

	TEST(ApplyingAPresetResetsOtherParametersButKeepsColor) {
		Brush brush;
		brush.color = { 0.2f, 0.4f, 0.6f, 1 };
		brush.set(BrushParameter::Roundness, 0.3f);
		brush.apply(*findBrushPreset(BrushType::Pencil));

		CHECK(brush.type == BrushType::Pencil);
		CHECK_EQUAL("Pencil", brush.presetName());
		CHECK_EQUAL(2.0f, brush.size());
		CHECK_EQUAL(0.0f, brush.hardness());
		CHECK_CLOSE(0.5f, brush.flow(), 1e-6f);
		CHECK_CLOSE(0.7f, brush.get(BrushParameter::MinSize), 1e-6f);
		CHECK_EQUAL(1.0f, brush.get(BrushParameter::Roundness));
		CHECK(brush.usesPressure(PressureTarget::Size));
		CHECK(brush.usesPressure(PressureTarget::Flow));
		CHECK(!brush.usesPressure(PressureTarget::Opacity));
		CHECK_CLOSE(0.4f, brush.color.g, 1e-6f);
	}

	TEST(CalligraphyIsAnAngledFlatTip) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Calligraphy));
		CHECK_EQUAL(45.0f, brush.get(BrushParameter::Angle));
		CHECK_CLOSE(0.25f, brush.get(BrushParameter::Roundness), 1e-6f);
	}

	TEST(AirBrushUsesOpacityAndFlowPressure) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Air));
		CHECK_EQUAL(48.0f, brush.size());
		CHECK(brush.usesPressure(PressureTarget::Opacity));
		CHECK(brush.usesPressure(PressureTarget::Flow));
		CHECK(!brush.usesPressure(PressureTarget::Size));
	}

	TEST(SmudgeLimitsItsSize) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Smudge));
		CHECK_CLOSE(0.8f, brush.get(BrushParameter::Smudge), 1e-6f);
		brush.setSize(900);
		CHECK_EQUAL(200.0f, brush.size());
	}
}

SUITE(PaperTextures) {
	halnf alphaVariance(const Document& document, int4 half) {
		document.settlePaint();
		halnf sum = 0;
		halnf squares = 0;
		int4 count = 0;
		for (int4 y = -half; y < half; y++) {
			for (int4 x = -half; x < half; x++) {
				const halnf alpha = (halnf) alphaAt(document, x, y);
				sum += alpha;
				squares += alpha * alpha;
				count++;
			}
		}
		const halnf mean = sum / (halnf) count;
		return squares / (halnf) count - mean * mean;
	}

	TEST(GrainAndDeckleAreDistinctTileableTextures) {
		const PaperTexture& grain = PaperTexture::grain();
		const PaperTexture& deckle = PaperTexture::deckle();
		CHECK_EQUAL(grain.at(3, 9), grain.at(3 + PaperTexture::kSize, 9 - PaperTexture::kSize));
		CHECK_CLOSE(0.5f, grain.mean(), 0.1f);
		int4 differing = 0;
		for (int4 index = 0; index < 64; index++)
			differing +=
				std::fabs(grain.at(index * 3 + 5, index + 11) - deckle.at(index * 3 + 5, index + 11)) > 0.05f ? 1 : 0;
		CHECK(differing > 32);
	}

	TEST(TextureBreaksUpASolidDab) {
		Brush brush;
		brush.setSize(60);
		brush.setHardness(1);
		Document smooth;
		smooth.stampBrushDab({ 0, 0 }, brush);

		brush.set(BrushParameter::Texture, 1);
		Document grainy;
		grainy.stampBrushDab({ 0, 0 }, brush);

		CHECK_CLOSE(0.0f, alphaVariance(smooth, 15), 1.0f);
		CHECK(alphaVariance(grainy, 15) > 200);
	}

	TEST(GrainIsAnchoredToTheCanvas) {
		Brush brush;
		brush.setSize(40);
		brush.set(BrushParameter::Texture, 1);
		Document first;
		Document second;
		first.stampBrushDab({ 0, 0 }, brush);
		second.stampBrushDab({ 0, 0 }, brush);
		second.stampBrushDab({ 2, 0 }, brush);
		int4 bothPainted = 0;
		int4 matchingHoles = 0;
		for (int4 x = -8; x < 8; x++) {
			const bool firstHole = alphaAt(first, x, 0) < 64;
			const bool secondHole = alphaAt(second, x, 0) < 64;
			bothPainted++;
			matchingHoles += firstHole == secondHole ? 1 : 0;
		}
		CHECK(matchingHoles * 4 >= bothPainted * 3);
	}

	TEST(JaggedBrushRoughensTheEdgeOnly) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Jagged));
		brush.setSize(120);
		brush.set(BrushParameter::Texture, 0);
		brush.setHardness(1);
		Document document;
		document.stampBrushDab({ 0, 0 }, brush);

		CHECK(alphaAt(document, 0, 0) > 0);
		int4 lengths[8];
		for (int4 ray = 0; ray < 8; ray++) {
			const halnf angle = 0.785398f * (halnf) ray;
			int4 length = 0;
			while (length < 70 && alphaAt(document, (int4) (std::cos(angle) * length), (int4) (std::sin(angle) * length)) > 0)
				length++;
			lengths[ray] = length;
		}
		const int4 shortest = *std::min_element(lengths, lengths + 8);
		const int4 longest = *std::max_element(lengths, lengths + 8);
		CHECK(shortest > 20);
		CHECK(longest - shortest >= 3);
	}
}

SUITE(MarkerBlending) {
	const uint1* pixelAt(const Document& document, int4 x, int4 y) {
		document.settlePaint();
		const Tile* tile = document.activeLayer()->tileIfExists({ floordiv(x, kTileSize), floordiv(y, kTileSize) });
		const int4 lx = x - floordiv(x, kTileSize) * kTileSize;
		const int4 ly = y - floordiv(y, kTileSize) * kTileSize;
		return tile->pixels() + ((ualni) ly * kTileSize + lx) * 4;
	}

	Brush halfOpaqueBrush(bool marker) {
		Brush brush;
		brush.setSize(20);
		brush.setOpacity(0.5f);
		brush.setMarkerBlend(marker);
		return brush;
	}

	TEST(MarkerPresetEnablesMarkerBlendingOnly) {
		for (const BrushPreset& preset : brushPresets()) {
			Brush brush;
			brush.apply(preset);
			CHECK_EQUAL(preset.type == BrushType::Marker, brush.markerBlend());
		}
	}

	TEST(OverlappingMarkerDabsKeepTheirOpacity) {
		Document document;
		const Brush brush = halfOpaqueBrush(true);
		for (int4 repeat = 0; repeat < 6; repeat++)
			document.stampBrushDab({ 0, 0 }, brush);
		CHECK_CLOSE(128, alphaAt(document, 0, 0), 1);
	}

	TEST(OpacityCapsOneStrokeAndSeparateStrokesBuildUp) {
		Document document;
		const Brush brush = halfOpaqueBrush(false);
		for (int4 repeat = 0; repeat < 6; repeat++)
			document.stampBrushDab({ 0, 0 }, brush);
		CHECK_CLOSE(128, alphaAt(document, 0, 0), 2);
		document.finishStroke();
		for (int4 stroke = 0; stroke < 5; stroke++) {
			document.stampBrushDab({ 0, 0 }, brush);
			document.finishStroke();
		}
		CHECK(alphaAt(document, 0, 0) > 250);
	}

	TEST(FlowBuildsUpTowardsOpacityWithinAStroke) {
		Document document;
		Brush brush = halfOpaqueBrush(false);
		brush.setFlow(0.3f);
		document.stampBrushDab({ 0, 0 }, brush);
		const int first = alphaAt(document, 0, 0);
		for (int4 repeat = 0; repeat < 20; repeat++)
			document.stampBrushDab({ 0, 0 }, brush);
		const int built = alphaAt(document, 0, 0);
		CHECK(first < 60);
		CHECK(built > first);
		CHECK(built <= 129);
	}

	TEST(MarkerTintsOpaquePaintWithoutCoveringIt) {
		Document document;
		Brush red = halfOpaqueBrush(false);
		red.setOpacity(1);
		red.color = { 1, 0, 0, 1 };
		document.stampBrushDab({ 0, 0 }, red);

		Brush blue = halfOpaqueBrush(true);
		blue.color = { 0, 0, 1, 1 };
		document.stampBrushDab({ 0, 0 }, blue);
		document.stampBrushDab({ 0, 0 }, blue);

		const uint1* pixel = pixelAt(document, 0, 0);
		CHECK_EQUAL(255, pixel[3]);
		CHECK_CLOSE(64, pixel[0], 2);
		CHECK_CLOSE(191, pixel[2], 2);
	}

	TEST(MarkerRaisesFaintPaintToItsOwnOpacity) {
		Document document;
		Brush faint = halfOpaqueBrush(false);
		faint.setOpacity(0.2f);
		document.stampBrushDab({ 0, 0 }, faint);

		document.stampBrushDab({ 0, 0 }, halfOpaqueBrush(true));
		CHECK_CLOSE(128, alphaAt(document, 0, 0), 1);
	}
}

SUITE(SmudgeBrush) {
	const uint1* pixelAt(const Document& document, int4 x, int4 y) {
		document.settlePaint();
		static const uint1 transparent[4] = {};
		const Tile* tile = document.activeLayer()->tileIfExists({ floordiv(x, kTileSize), floordiv(y, kTileSize) });
		if (!tile) return transparent;
		const int4 lx = x - floordiv(x, kTileSize) * kTileSize;
		const int4 ly = y - floordiv(y, kTileSize) * kTileSize;
		return tile->pixels() + ((ualni) ly * kTileSize + lx) * 4;
	}

	void fillRect(Document & document, int4 x0, int4 x1, RGBA color) {
		Brush square;
		square.setSize(4);
		square.setHardness(1);
		square.setSpacing(0.25f);
		square.color = color;
		for (int4 y = -16; y <= 16; y += 1)
			for (int4 x = x0; x <= x1; x += 1)
				document.stampBrushDab({ (halnf) x, (halnf) y }, square);
		document.finishStroke();
	}

	Brush smudge() {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Smudge));
		brush.setSize(16);
		brush.setFlow(1);
		brush.set(BrushParameter::Blur, 0);
		return brush;
	}

	void drag(Document & document, const Brush& brush, int4 fromX, int4 toX) {
		for (int4 x = fromX; x <= toX; x++)
			document.stampBrushDab(Vec2F((halnf) x, 0.0f), brush);
		document.finishStroke("Smudge");
	}

	TEST(SmudgeDragsPaintIntoEmptyCanvas) {
		Document document;
		fillRect(document, -40, 0, { 1, 0, 0, 1 });
		CHECK_EQUAL(0, pixelAt(document, 14, 0)[3]);

		drag(document, smudge(), -6, 14);
		const uint1* dragged = pixelAt(document, 14, 0);
		CHECK(dragged[3] > 60);
		CHECK(dragged[0] > 60);
		CHECK_EQUAL(0, dragged[2]);
	}

	TEST(SmudgeNeverAddsTheBrushColour) {
		Document document;
		fillRect(document, -40, 40, { 1, 0, 0, 1 });
		Brush brush = smudge();
		brush.color = { 0, 0, 1, 1 };
		drag(document, brush, -20, 20);
		CHECK_EQUAL(0, pixelAt(document, 0, 0)[2]);
		CHECK_EQUAL(255, pixelAt(document, 0, 0)[0]);
	}

	TEST(ResaturationBlendsTheBrushColourIn) {
		Document document;
		fillRect(document, -40, 40, { 1, 0, 0, 1 });
		Brush brush = smudge();
		brush.color = { 0, 0, 1, 1 };
		brush.set(BrushParameter::Resaturation, 0.01f);
		drag(document, brush, -20, 20);
		CHECK(pixelAt(document, 18, 0)[2] > 20);
	}

	TEST(BlurBrushSoftensAHardEdge) {
		Document document;
		fillRect(document, -40, 0, { 1, 1, 1, 1 });
		const uint1 before = pixelAt(document, 4, 0)[3];
		Brush brush = smudge();
		brush.set(BrushParameter::Smudge, 0);
		brush.set(BrushParameter::Blur, 4);
		for (int4 repeat = 0; repeat < 4; repeat++)
			drag(document, brush, 0, 0);
		const uint1 after = pixelAt(document, 4, 0)[3];
		CHECK_EQUAL(0, before);
		CHECK(after > 20);
		CHECK(after < 200);
	}

	TEST(LinearHackMixesColoursBrighter) {
		uint1 mixedRed[2] = {};
		for (bool linear : { false, true }) {
			Document document;
			fillRect(document, -40, 0, { 1, 0, 0, 1 });
			fillRect(document, 1, 40, { 0, 1, 0, 1 });
			Brush brush = smudge();
			brush.set(BrushParameter::Smudge, 0);
			brush.set(BrushParameter::Blur, 6);
			brush.setLinearMixing(linear);
			drag(document, brush, 0, 0);
			mixedRed[linear] = pixelAt(document, 1, 0)[0];
		}
		CHECK(mixedRed[0] > 60);
		CHECK(mixedRed[1] > mixedRed[0] + 30);
	}

	TEST(SmudgeRefusesAnEmptyLayer) {
		Document document;
		CHECK_EQUAL("You can't smudge a empty layer", document.strokeRefusal(smudge()));
		fillRect(document, 0, 4, { 1, 0, 0, 1 });
		CHECK_EQUAL("", document.strokeRefusal(smudge()));
	}
}

SUITE(EraserMode) {
	TEST(AnyPaintingBrushCanErase) {
		for (const BrushPreset& preset : brushPresets()) {
			Brush brush;
			brush.apply(preset);
			brush.setEraserMode(true);
			CHECK_EQUAL(preset.type != BrushType::Smudge, brush.erases());
		}
	}

	TEST(EraserModeKeepsTheBrushShapeAndRemovesPaint) {
		Document document;
		Brush paint;
		paint.setSize(60);
		document.stampBrushDab({ 0, 0 }, paint);
		document.finishStroke();

		Brush pencil;
		pencil.apply(*findBrushPreset(BrushType::Calligraphy));
		pencil.setSize(40);
		pencil.setEraserMode(true);
		document.stampBrushDab({ 0, 0 }, pencil);

		CHECK_EQUAL(0, alphaAt(document, 0, 0));
		CHECK(alphaAt(document, 0, 15) > 200);
		CHECK_EQUAL("You can't erase on a alpha locked layer", [&]() {
			document.activeLayer()->setAlphaLocked(true);
			return document.strokeRefusal(pencil);
		}());
	}

	TEST(ApplyingAPresetLeavesEraserMode) {
		Brush brush;
		brush.setEraserMode(true);
		brush.apply(*findBrushPreset(BrushType::Ink));
		CHECK(!brush.erases());
	}
}
