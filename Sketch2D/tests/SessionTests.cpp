#include "Session.hpp"

#include "UnitTest++/UnitTest++.h"

#include <tuple>
#include <vector>

#include <cstring>
#include <filesystem>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	struct PaintedDocument {
		Document document;
		Brush brush;
		ViewSettings settings;

		PaintedDocument() {
			brush.apply(*findBrushPreset(BrushType::Calligraphy));
			brush.color = { 0.25f, 0.5f, 0.75f, 1 };
			brush.setSize(37);
			brush.set(BrushParameter::Texture, 0.4f);
			brush.setUsesPressure(PressureTarget::Opacity, true);
			brush.setStabilizer(0.7f);
			brush.setEraserMode(true);
			brush.setMarkerBlend(true);

			Brush paint;
			paint.setSize(80);
			document.stampBrushDab({ 10, 10 }, paint);
			document.stampBrushDab({ 300, 40 }, paint);
			document.finishStroke();
			Layer* top = document.addLayer("Ink \"top\"");
			top->setOpacity(0.5f);
			top->setBlend(BlendMode::Multiply);
			top->setAlphaLocked(true);
			document.stampBrushDab({ -50, 5 }, paint);
			document.finishStroke();

			document.setZoom(2.5f);
			document.setPanWorld({ -120, 33 });
			settings.useBrushStabilizer = false;
			settings.globalStabilizer = 0.4f;
			settings.viewStiffness = 0.75f;
			settings.showChannel = true;
			settings.channel = ViewChannel::Green;
			document.setBackground({ { 30, 0.2f, 0.3f }, true });
		}

		SessionSnapshot capture() const {
			return SessionSnapshot::capture(document, brush, settings, { "abc", "Radeon", "today", { 1600, 1000 }, 1.25f });
		}
	};

	bool samePixels(const Document& left, const Document& right) {
		if (left.layers().size() != right.layers().size()) return false;
		for (size_t index = 0; index < left.layers().size(); index++) {
			const Layer& a = *left.layers()[index];
			const Layer& b = *right.layers()[index];
			if (a.tileCount() != b.tileCount()) return false;
			bool equal = true;
			a.forEachTile([&](const TileCoord& coord, Tile* tile) {
				const Tile* other = b.tileIfExists(coord);
				equal = equal && other && std::memcmp(tile->pixels(), other->pixels(), kTileSize * kTileSize * 4) == 0;
			});
			if (!equal) return false;
		}
		return true;
	}
}

SUITE(Sketch2DSession) {
	TEST(SnapshotTextRoundTripsEveryRecordedSetting) {
		const PaintedDocument source;
		const std::optional<SessionSnapshot> parsed = SessionSnapshot::parse(source.capture().serialize());
		REQUIRE CHECK(parsed.has_value());

		CHECK(parsed->brush == source.brush);
		CHECK_EQUAL(2.5f, parsed->view.zoom);
		CHECK_EQUAL(-120.0f, parsed->view.pan.x);
		CHECK_EQUAL(1.25f, parsed->environment.contentScale);
		CHECK_EQUAL("Radeon", parsed->environment.device);
		CHECK(!parsed->settings.useBrushStabilizer);
		CHECK_EQUAL(0.4f, parsed->settings.globalStabilizer);
		CHECK_CLOSE(0.75f, parsed->settings.viewStiffness, 0.0001f);
		CHECK(parsed->settings.channel == ViewChannel::Green);
		CHECK(parsed->document.background == source.document.background());
		REQUIRE CHECK_EQUAL(2u, parsed->document.layers.size());
		const Layer::Properties& top = parsed->document.layers[1].properties;
		CHECK_EQUAL("Ink \"top\"", top.name);
		CHECK_EQUAL(0.5f, top.opacity);
		CHECK(top.blend == BlendMode::Multiply);
		CHECK(top.alphaLocked);
		CHECK_EQUAL(source.document.activeLayer()->id(), parsed->document.active);
	}

	TEST(SnapshotRejectsUnknownLines) {
		CHECK(!SessionSnapshot::parse("sketch2d-session 1\nwhat 1\n").has_value());
		CHECK(!SessionSnapshot::parse("not a session\n").has_value());
		CHECK(!SessionSnapshot::parse("sketch2d-session 1\nbrush.parameter nonsense 3\n").has_value());
	}

	TEST(LayerTilesRoundTripExactly) {
		const PaintedDocument source;
		const Layer::State original = source.document.layers().front()->state();
		Layer::State loaded;
		REQUIRE CHECK(parseLayerTiles(serializeLayerTiles(original), loaded));
		CHECK_EQUAL(original.tiles.size(), loaded.tiles.size());
		CHECK(!parseLayerTiles("garbage", loaded));
	}

	std::string storedTiles(const std::string& header, int4 size, const std::vector<TileCoord>& coords) {
		std::string bytes = header;
		for (size_t index = 0; index < coords.size(); index++) {
			const int32_t xy[2] = { coords[index].x, coords[index].y };
			bytes.append(reinterpret_cast<const char*>(xy), sizeof(xy));
			std::string pixels((size_t) size * size * 4, '\0');
			for (int4 y = 0; y < size; y++)
				for (int4 x = 0; x < size; x++) {
					const size_t at = ((size_t) y * size + x) * 4;
					pixels[at + 0] = (char) (index + 1);
					pixels[at + 1] = (char) (x % 256);
					pixels[at + 2] = (char) (y % 256);
					pixels[at + 3] = (char) 255;
				}
			bytes += pixels;
		}
		return bytes;
	}

	const uint1* storedPixel(const Layer::State& layer, int4 x, int4 y) {
		const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
		auto found = layer.tiles.presents(coord.key());
		if (!found.isValid()) return nullptr;
		const Tile* tile = layer.tiles.getSlotVal(found).tile.get();
		return tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
	}

	TEST(LegacyTilesLoadAtAnyTileSize) {
		Layer::State loaded;
		REQUIRE CHECK(parseLayerTiles(storedTiles("sketch2d-tiles 1\n", 256, { { 0, 0 }, { -1, 2 } }), loaded));
		for (const auto& [x, y, owner] : std::vector<std::tuple<int4, int4, int4>>{
					 { 0, 0, 1 }, { 255, 255, 1 }, { 130, 70, 1 }, { -256, 512, 2 }, { -1, 767, 2 }, { -100, 600, 2 } }) {
			const uint1* pixel = storedPixel(loaded, x, y);
			REQUIRE CHECK(pixel != nullptr);
			CHECK_EQUAL(owner, (int) pixel[0]);
			CHECK_EQUAL(((x % 256) + 256) % 256, (int) pixel[1]);
			CHECK_EQUAL(((y % 256) + 256) % 256, (int) pixel[2]);
		}
		CHECK(storedPixel(loaded, 300, 0) == nullptr || storedPixel(loaded, 300, 0)[3] == 0);
		const ualni expected = 2 * (ualni) std::max(1, 256 / kTileSize) * std::max(1, 256 / kTileSize);
		if (kTileSize <= 256) CHECK_EQUAL(expected, loaded.tiles.size());
	}

	TEST(TilesOfAnotherSizeAreResliced) {
		Layer::State loaded;
		REQUIRE CHECK(parseLayerTiles(storedTiles("sketch2d-tiles 2 128\n", 128, { { 3, -2 } }), loaded));
		Layer::State older;
		CHECK(parseLayerTiles(storedTiles("leo-tiles 2 128\n", 128, { { 3, -2 } }), older));
		CHECK(parseLayerTiles(storedTiles("leo-tiles 1\n", 256, { { 0, 0 } }), older));
		const uint1* pixel = storedPixel(loaded, 3 * 128 + 5, -2 * 128 + 7);
		REQUIRE CHECK(pixel != nullptr);
		CHECK_EQUAL(1, (int) pixel[0]);
		CHECK_EQUAL(5, (int) pixel[1]);
		CHECK_EQUAL(7, (int) pixel[2]);
		CHECK(!parseLayerTiles("sketch2d-tiles 2 0\n", loaded));
	}

	TEST(SavedSessionRestoresTheSameDocumentViewAndBrush) {
		const PaintedDocument source;
		RecordedSession session;
		session.snapshot = source.capture();
		session.input.windowSize = { 1600, 1000 };
		RecordedInput move;
		move.timeMs = 3.5;
		move.vector = { 40, 50 };
		session.input.events.push_back(move);
		session.recordedFrames = "frame\twall\n0\t1.5\n";

		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "sketch2d-session-test";
		std::filesystem::remove_all(directory);
		REQUIRE CHECK(session.save(directory.string()));

		const std::optional<RecordedSession> loaded = RecordedSession::load(directory.string());
		REQUIRE CHECK(loaded.has_value());
		CHECK_EQUAL(1u, loaded->input.events.size());
		CHECK_EQUAL(session.recordedFrames, loaded->recordedFrames);

		Document document;
		Brush brush;
		ViewSettings settings;
		loaded->snapshot.applyTo(document, brush, settings);
		CHECK(samePixels(source.document, document));
		CHECK(brush == source.brush);
		CHECK_EQUAL(2.5f, document.zoom());
		CHECK_EQUAL(33.0f, document.panWorld().y);
		CHECK_EQUAL(1u, document.history().entries().size());
		CHECK_EQUAL("Ink \"top\"", document.activeLayer()->name());

		Layer* added = document.addLayer("after");
		CHECK(added->id() > document.layers()[1]->id());
		std::filesystem::remove_all(directory);
	}

	TEST(MissingLayerFileFailsTheLoad) {
		const PaintedDocument source;
		RecordedSession session;
		session.snapshot = source.capture();
		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "sketch2d-session-broken";
		std::filesystem::remove_all(directory);
		REQUIRE CHECK(session.save(directory.string()));
		std::filesystem::remove_all(directory / "layers");
		CHECK(!RecordedSession::load(directory.string()).has_value());
		std::filesystem::remove_all(directory);
	}
}
