#include "Session.hpp"

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace tp::sketch2d {

	namespace {
		constexpr const char* kLegacyTileMagic = "sketch2d-tiles 1\n";
		constexpr const char* kTileMagic = "sketch2d-tiles 2 ";
		constexpr const char* kOldTileMagic = "leo-tiles 1\n";
		constexpr const char* kOldSizedTileMagic = "leo-tiles 2 ";
		constexpr int4 kLegacyTileSize = 256;

		size_t tileBytes(int4 size) { return (size_t) size * (size_t) size * 4; }

		void placeStoredTile(Layer::State& layer, TileCoord stored, int4 storedSize, const uint1* pixels) {
			const int4 originX = stored.x * storedSize;
			const int4 originY = stored.y * storedSize;
			const int4 tx0 = floordiv(originX, kTileSize);
			const int4 ty0 = floordiv(originY, kTileSize);
			const int4 tx1 = floordiv(originX + storedSize - 1, kTileSize);
			const int4 ty1 = floordiv(originY + storedSize - 1, kTileSize);

			for (int4 ty = ty0; ty <= ty1; ty++)
				for (int4 tx = tx0; tx <= tx1; tx++) {
					const TileCoord coord = { tx, ty };
					auto found = layer.tiles.presents(coord.key());
					Layer::TilePointer tile = found.isValid() ? layer.tiles.getSlotVal(found).tile : std::make_shared<Tile>();
					if (!found.isValid()) layer.tiles.put(coord.key(), { coord, tile });

					const int4 x0 = std::max(originX, tx * kTileSize);
					const int4 y0 = std::max(originY, ty * kTileSize);
					const int4 x1 = std::min(originX + storedSize, (tx + 1) * kTileSize);
					const int4 y1 = std::min(originY + storedSize, (ty + 1) * kTileSize);
					for (int4 y = y0; y < y1; y++) {
						const uint1* source = pixels + ((size_t) (y - originY) * storedSize + (size_t) (x0 - originX)) * 4;
						uint1* target =
							tile->pixels() + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x0 - tx * kTileSize)) * 4;
						std::memcpy(target, source, (size_t) (x1 - x0) * 4);
					}
					tile->clearDirty();
				}
		}

		void dropTransparentTiles(Layer::State& layer) {
			Map<ualni, Layer::TileSlot> kept;
			for (auto entry : layer.tiles)
				if (!entry->val.tile->isTransparent()) kept.put(entry->key, entry->val);
			layer.tiles = kept;
		}

		std::string quoted(const std::string& text) {
			std::string escaped = "\"";
			for (char character : text) {
				if (character == '"' || character == '\\') escaped += '\\';
				escaped += character;
			}
			return escaped + "\"";
		}

		std::string readQuoted(std::istringstream& input) {
			input >> std::ws;
			if (input.peek() != '"') return {};
			input.get();
			std::string text;
			for (char character; input.get(character);) {
				if (character == '\\') {
					input.get(character);
					text += character;
					continue;
				}
				if (character == '"') break;
				text += character;
			}
			return text;
		}

		std::string number(double value) {
			char text[48];
			std::snprintf(text, sizeof(text), "%.9g", value);
			return text;
		}

		std::string line(const std::string& key, const std::vector<std::string>& values) {
			std::string text = key;
			for (const std::string& value : values)
				text += " " + value;
			return text + "\n";
		}

		void appendBrush(std::string& text, const Brush& brush) {
			text += line("brush.type", { std::to_string((int) brush.type) });
			text += line("brush.preset", { quoted(brush.presetName()) });
			text += line(
				"brush.color", { number(brush.color.r), number(brush.color.g), number(brush.color.b), number(brush.color.a) }
			);
			text += line("brush.size_limits", { number(brush.minimumSize()), number(brush.maximumSize()) });
			text += line("brush.stabilizer", { number(brush.stabilizer()) });
			text += line("brush.marker_blend", { std::to_string((int) brush.markerBlend()) });
			text += line("brush.linear_mixing", { std::to_string((int) brush.linearMixing()) });
			text += line("brush.eraser_mode", { std::to_string((int) brush.eraserMode()) });
			for (size_t index = 0; index < kBrushParameterCount; index++) {
				const BrushParameter parameter = (BrushParameter) index;
				text += line("brush.parameter", { brushParameterInfo(parameter).name, number(brush.get(parameter)) });
			}
			for (size_t index = 0; index < kPressureTargetCount; index++) {
				const PressureTarget target = (PressureTarget) index;
				text +=
					line("brush.pressure", { pressureTargetName(target), std::to_string((int) brush.usesPressure(target)) });
			}
		}

		void appendSettings(std::string& text, const ViewSettings& settings) {
			text += line(
				"view.board",
				{ number(settings.boardColor.hueDegrees),
					number(settings.boardColor.saturation),
					number(settings.boardColor.value) }
			);
			text += line("view.canvas_shadow", { std::to_string((int) settings.canvasShadow) });
			text += line("view.stiffness", { number(settings.viewStiffness) });
			text += line("view.zoom_limits", { number(settings.zoomInLimit), number(settings.zoomOutLimit) });
			text +=
				line("view.channel", { std::to_string((int) settings.showChannel), std::to_string((int) settings.channel) });
			text += line(
				"view.stabilizer", { std::to_string((int) settings.useBrushStabilizer), number(settings.globalStabilizer) }
			);
			const BrushCursorSettings& cursor = settings.cursor;
			text += line(
				"view.cursor",
				{ std::to_string((int) cursor.style),
					std::to_string((int) cursor.contourForEraserAndSmudge),
					std::to_string((int) cursor.crosshairForSmallContour),
					std::to_string((int) cursor.spotlight),
					number(cursor.spotlightColor.hueDegrees),
					number(cursor.spotlightColor.saturation),
					number(cursor.spotlightColor.value),
					number(cursor.spotlightAlpha),
					std::to_string((int) cursor.showForPen) }
			);
			text += line(
				"view.line",
				{ std::to_string((int) settings.line.holdForLine),
					number(settings.line.holdSeconds),
					std::to_string((int) settings.line.mode) }
			);
			const PerspectiveSettings& perspective = settings.perspective;
			text += line(
				"view.perspective",
				{ std::to_string((int) perspective.type),
					std::to_string((int) perspective.locked),
					std::to_string((int) perspective.placed),
					number(perspective.isometricDegrees),
					number(perspective.points[0].x),
					number(perspective.points[0].y),
					number(perspective.points[1].x),
					number(perspective.points[1].y),
					number(perspective.points[2].x),
					number(perspective.points[2].y) }
			);
			const SymmetrySettings& symmetry = settings.symmetry;
			text += line(
				"view.symmetry",
				{ std::to_string((int) symmetry.enabled),
					std::to_string((int) symmetry.visible),
					std::to_string((int) symmetry.locked),
					number(symmetry.center.x),
					number(symmetry.center.y),
					number(symmetry.angle) }
			);
			const GradientSettings& gradient = settings.gradient;
			text += line(
				"view.gradient",
				{ std::to_string((int) gradient.shape),
					std::to_string((int) gradient.toTransparent),
					std::to_string((int) gradient.eraser) }
			);
			const BucketSettings& bucket = settings.bucket;
			text += line(
				"view.bucket",
				{ number(bucket.threshold),
					std::to_string((int) bucket.onlyAlpha),
					std::to_string((int) bucket.allLayers),
					std::to_string(bucket.spread) }
			);
			const PickerSettings& picker = settings.picker;
			text += line(
				"view.picker",
				{ std::to_string((int) picker.sampleAllLayers),
					std::to_string((int) picker.sampleOpacity),
					std::to_string((int) picker.style) }
			);
		}

		void appendDocument(std::string& text, const DocumentState& document) {
			const CanvasBackground& background = document.background;
			text += line(
				"document.background",
				{ number(background.color.hueDegrees),
					number(background.color.saturation),
					number(background.color.value),
					std::to_string((int) background.transparent) }
			);
			const CanvasFrame& frame = document.frame;
			text += line(
				"document.frame",
				{ std::to_string(frame.left),
					std::to_string(frame.top),
					std::to_string(frame.right),
					std::to_string(frame.bottom),
					std::to_string((int) frame.infinite) }
			);
			text += line("document.active", { std::to_string(document.active) });
			for (const Layer::State& layer : document.layers) {
				const Layer::Properties& properties = layer.properties;
				text += line(
					"layer",
					{ std::to_string(layer.id),
						quoted(properties.name),
						std::to_string((int) properties.visible),
						number(properties.opacity),
						std::to_string((int) properties.blend),
						std::to_string((int) properties.clipped),
						std::to_string((int) properties.locked),
						std::to_string((int) properties.alphaLocked),
						std::to_string((int) properties.selected),
						std::to_string(layer.tiles.size()) }
				);
			}
		}

		bool readBrushParameter(std::istringstream& fields, Brush& brush) {
			std::string name;
			halnf value = 0;
			fields >> name >> value;
			for (size_t index = 0; index < kBrushParameterCount; index++) {
				const BrushParameter parameter = (BrushParameter) index;
				if (name != brushParameterInfo(parameter).name) continue;
				brush.set(parameter, value);
				return true;
			}
			return false;
		}

		bool readBrushPressure(std::istringstream& fields, Brush& brush) {
			std::string name;
			int uses = 0;
			fields >> name >> uses;
			for (size_t index = 0; index < kPressureTargetCount; index++) {
				const PressureTarget target = (PressureTarget) index;
				if (name != pressureTargetName(target)) continue;
				brush.setUsesPressure(target, uses != 0);
				return true;
			}
			return false;
		}

		std::string layerFile(ualni id) { return "layers/layer-" + std::to_string(id) + ".tiles"; }

		std::string readBytes(const std::filesystem::path& path) {
			std::ifstream file(path, std::ios::binary);
			std::ostringstream bytes;
			bytes << file.rdbuf();
			return bytes.str();
		}

		bool writeBytes(const std::filesystem::path& path, const std::string& bytes) {
			std::ofstream file(path, std::ios::binary);
			file.write(bytes.data(), (std::streamsize) bytes.size());
			return (bool) file;
		}
	}

	SessionSnapshot SessionSnapshot::capture(
		const Document& document, const Brush& brush, const ViewSettings& settings, SessionEnvironment environment
	) {
		SessionSnapshot snapshot;
		snapshot.environment = std::move(environment);
		snapshot.view = { document.panWorld(), document.zoom(), document.rotation() };
		snapshot.brush = brush;
		snapshot.settings = settings;
		snapshot.document = document.state();
		return snapshot;
	}

	void SessionSnapshot::applyTo(Document& document, Brush& target, ViewSettings& targetSettings) const {
		document.load(this->document, "Recorded session");
		document.setZoom(view.zoom);
		document.setRotation(view.rotation);
		document.setPanWorld(view.pan);
		target = brush;
		targetSettings = settings;
	}

	std::string SessionSnapshot::serialize() const {
		std::string text = std::string(kHeader) + "\n";
		text += line("environment.revision", { quoted(environment.revision) });
		text += line("environment.device", { quoted(environment.device) });
		text += line("environment.date", { quoted(environment.date) });
		text += line("environment.window", { number(environment.windowSize.x), number(environment.windowSize.y) });
		text += line("environment.scale", { number(environment.contentScale) });
		text += line("view.pan", { number(view.pan.x), number(view.pan.y) });
		text += line("view.zoom", { number(view.zoom) });
		text += line("view.rotation", { number(view.rotation) });
		appendBrush(text, brush);
		appendSettings(text, settings);
		text += serializeDocument(document);
		return text;
	}

	std::string serializeDocument(const DocumentState& document) {
		std::string text;
		appendDocument(text, document);
		return text;
	}

	bool parseDocumentLine(const std::string& key, std::istringstream& fields, DocumentState& document) {
		int flag = 0;
		if (key == "document.background") {
			CanvasBackground& background = document.background;
			fields >> background.color.hueDegrees >> background.color.saturation >> background.color.value >> flag;
			background.transparent = flag != 0;
		} else if (key == "document.frame") {
			CanvasFrame& frame = document.frame;
			fields >> frame.left >> frame.top >> frame.right >> frame.bottom >> flag;
			frame.infinite = flag != 0;
		} else if (key == "document.active") fields >> document.active;
		else if (key == "layer") {
			Layer::State layer;
			Layer::Properties& properties = layer.properties;
			int visible = 1, blend = 0, clipped = 0, locked = 0, alphaLocked = 0, selected = 0;
			size_t tiles = 0;
			fields >> layer.id;
			properties.name = readQuoted(fields);
			fields >> visible >> properties.opacity >> blend >> clipped >> locked >> alphaLocked >> selected >> tiles;
			properties.visible = visible != 0;
			properties.blend = (BlendMode) blend;
			properties.clipped = clipped != 0;
			properties.locked = locked != 0;
			properties.alphaLocked = alphaLocked != 0;
			properties.selected = selected != 0;
			document.layers.push_back(std::move(layer));
		} else return false;
		return true;
	}

	std::string readFileBytes(const std::string& path) { return readBytes(path); }

	bool writeFileBytes(const std::string& path, const std::string& bytes) { return writeBytes(path, bytes); }

	bool saveLayerTiles(const std::string& directory, const DocumentState& document) {
		const std::filesystem::path root(directory);
		std::error_code error;
		std::filesystem::create_directories(root / "layers", error);
		if (error) return false;

		bool written = true;
		for (const Layer::State& layer : document.layers)
			written = written && writeBytes(root / layerFile(layer.id), serializeLayerTiles(layer));
		return written;
	}

	bool loadLayerTiles(const std::string& directory, DocumentState& document) {
		const std::filesystem::path root(directory);
		for (Layer::State& layer : document.layers) {
			if (!parseLayerTiles(readBytes(root / layerFile(layer.id)), layer)) return false;
		}
		return true;
	}

	std::optional<SessionSnapshot> SessionSnapshot::parse(const std::string& text) {
		std::istringstream input(text);
		std::string row;
		if (!std::getline(input, row) || (row != kHeader && row != kLegacyHeader)) return std::nullopt;

		SessionSnapshot snapshot;
		snapshot.document.layers.clear();
		Brush& brush = snapshot.brush;
		while (std::getline(input, row)) {
			if (row.empty()) continue;
			std::istringstream fields(row);
			std::string key;
			fields >> key;
			int flag = 0;
			int other = 0;

			if (key == "environment.revision") snapshot.environment.revision = readQuoted(fields);
			else if (key == "environment.device") snapshot.environment.device = readQuoted(fields);
			else if (key == "environment.date") snapshot.environment.date = readQuoted(fields);
			else if (key == "environment.window")
				fields >> snapshot.environment.windowSize.x >> snapshot.environment.windowSize.y;
			else if (key == "environment.scale") fields >> snapshot.environment.contentScale;
			else if (key == "view.pan") fields >> snapshot.view.pan.x >> snapshot.view.pan.y;
			else if (key == "view.zoom") fields >> snapshot.view.zoom;
			else if (key == "view.rotation") fields >> snapshot.view.rotation;
			else if (key == "brush.type") {
				fields >> other;
				brush.type = (BrushType) other;
			} else if (key == "brush.preset") brush.setPresetName(readQuoted(fields));
			else if (key == "brush.color") fields >> brush.color.r >> brush.color.g >> brush.color.b >> brush.color.a;
			else if (key == "brush.size_limits") {
				halnf minimum = 1;
				halnf maximum = Brush::kMaximumSize;
				fields >> minimum >> maximum;
				brush.setSizeLimits(minimum, maximum);
			} else if (key == "brush.stabilizer") {
				halnf amount = 0;
				fields >> amount;
				brush.setStabilizer(amount);
			} else if (key == "brush.marker_blend") {
				fields >> flag;
				brush.setMarkerBlend(flag != 0);
			} else if (key == "brush.linear_mixing") {
				fields >> flag;
				brush.setLinearMixing(flag != 0);
			} else if (key == "brush.eraser_mode") {
				fields >> flag;
				brush.setEraserMode(flag != 0);
			} else if (key == "brush.parameter") {
				if (!readBrushParameter(fields, brush)) return std::nullopt;
			} else if (key == "brush.pressure") {
				if (!readBrushPressure(fields, brush)) return std::nullopt;
			} else if (key == "view.board") {
				HueColor& board = snapshot.settings.boardColor;
				fields >> board.hueDegrees >> board.saturation >> board.value;
			} else if (key == "view.canvas_shadow") {
				fields >> flag;
				snapshot.settings.canvasShadow = flag != 0;
			} else if (key == "view.stiffness") fields >> snapshot.settings.viewStiffness;
			else if (key == "view.zoom_limits") fields >> snapshot.settings.zoomInLimit >> snapshot.settings.zoomOutLimit;
			else if (key == "view.channel") {
				fields >> flag >> other;
				snapshot.settings.showChannel = flag != 0;
				snapshot.settings.channel = (ViewChannel) other;
			} else if (key == "view.cursor") {
				BrushCursorSettings& cursor = snapshot.settings.cursor;
				int4 style = 0;
				int4 eraser = 1;
				int4 small = 1;
				int4 spotlight = 0;
				int4 pen = 1;
				fields >> style >> eraser >> small >> spotlight >> cursor.spotlightColor.hueDegrees >>
					cursor.spotlightColor.saturation >> cursor.spotlightColor.value >> cursor.spotlightAlpha >> pen;
				if (style < 0 || style >= kBrushCursorStyleCount) return std::nullopt;
				cursor.style = (BrushCursorStyle) style;
				cursor.contourForEraserAndSmudge = eraser != 0;
				cursor.crosshairForSmallContour = small != 0;
				cursor.spotlight = spotlight != 0;
				cursor.showForPen = pen != 0;
			} else if (key == "view.line") {
				int4 hold = 1;
				halnf seconds = 0.5f;
				int4 mode = 0;
				fields >> hold >> seconds;
				if (!(fields >> mode)) mode = 0;
				if (seconds < 0.05f || seconds > 5 || mode < 0 || mode > (int4) LineMode::Perspective) return std::nullopt;
				snapshot.settings.line = { hold != 0, seconds, (LineMode) mode };
			} else if (key == "view.perspective") {
				PerspectiveSettings& perspective = snapshot.settings.perspective;
				int4 type = 0, locked = 0, placed = 0;
				fields >> type >> locked >> placed >> perspective.isometricDegrees;
				for (Vec2F& point : perspective.points)
					fields >> point.x >> point.y;
				if (type < 0 || type > (int4) PerspectiveType::Radial || !std::isfinite(perspective.isometricDegrees))
					return std::nullopt;
				perspective.type = (PerspectiveType) type;
				perspective.locked = locked != 0;
				perspective.placed = placed != 0;
			} else if (key == "view.symmetry") {
				SymmetrySettings& symmetry = snapshot.settings.symmetry;
				int4 enabled = 0, visible = 1, locked = 0;
				fields >> enabled >> visible >> locked >> symmetry.center.x >> symmetry.center.y >> symmetry.angle;
				if (!std::isfinite(symmetry.center.x) || !std::isfinite(symmetry.center.y) || !std::isfinite(symmetry.angle))
					return std::nullopt;
				symmetry.enabled = enabled != 0;
				symmetry.visible = visible != 0;
				symmetry.locked = locked != 0;
			} else if (key == "view.gradient") {
				int4 shape = 0;
				int4 transparent = 0;
				int4 eraser = 0;
				fields >> shape >> transparent >> eraser;
				if (shape < 0 || shape > (int4) GradientShape::Radial) return std::nullopt;
				snapshot.settings.gradient = { (GradientShape) shape, transparent != 0, eraser != 0 };
			} else if (key == "view.bucket") {
				BucketSettings& bucket = snapshot.settings.bucket;
				int4 alpha = 0;
				int4 all = 1;
				fields >> bucket.threshold >> alpha >> all >> bucket.spread;
				if (bucket.threshold < 0 || bucket.threshold > 1 || bucket.spread < 0 || bucket.spread > 64)
					return std::nullopt;
				bucket.onlyAlpha = alpha != 0;
				bucket.allLayers = all != 0;
			} else if (key == "view.picker") {
				int4 all = 1;
				int4 opacity = 0;
				int4 style = 1;
				fields >> all >> opacity >> style;
				if (style < 0 || style > (int4) PickerStyle::Magnifier) return std::nullopt;
				snapshot.settings.picker = { all != 0, opacity != 0, (PickerStyle) style };
			} else if (key == "view.stabilizer") {
				fields >> flag >> snapshot.settings.globalStabilizer;
				snapshot.settings.useBrushStabilizer = flag != 0;
			} else if (!parseDocumentLine(key, fields, snapshot.document)) return std::nullopt;

			if (fields.fail()) return std::nullopt;
		}
		return snapshot;
	}

	std::string serializeLayerTiles(const Layer::State& layer) {
		std::string bytes = std::string(kTileMagic) + std::to_string(kTileSize) + "\n";
		for (auto entry : layer.tiles) {
			const Layer::TileSlot& slot = entry->val;
			if (!slot.tile) continue;
			const int32_t coordinates[2] = { slot.coord.x, slot.coord.y };
			bytes.append(reinterpret_cast<const char*>(coordinates), sizeof(coordinates));
			bytes.append(reinterpret_cast<const char*>(slot.tile->pixels()), tileBytes(kTileSize));
		}
		return bytes;
	}

	bool parseLayerTiles(const std::string& bytes, Layer::State& layer) {
		size_t header = 0;
		int4 storedSize = 0;
		const auto startsWith = [&bytes](const char* magic) { return bytes.compare(0, std::strlen(magic), magic) == 0; };
		const char* sized = startsWith(kTileMagic)           ? kTileMagic
												: startsWith(kOldSizedTileMagic) ? kOldSizedTileMagic
																												 : nullptr;
		if (startsWith(kLegacyTileMagic) || startsWith(kOldTileMagic)) {
			header = std::strlen(startsWith(kOldTileMagic) ? kOldTileMagic : kLegacyTileMagic);
			storedSize = kLegacyTileSize;
		} else if (sized) {
			const size_t end = bytes.find('\n');
			if (end == std::string::npos) return false;
			storedSize = std::atoi(bytes.substr(std::strlen(sized), end - std::strlen(sized)).c_str());
			header = end + 1;
		} else {
			return false;
		}
		if (storedSize <= 0) return false;

		const size_t record = sizeof(int32_t) * 2 + tileBytes(storedSize);
		if ((bytes.size() - header) % record != 0) return false;

		for (size_t offset = header; offset < bytes.size(); offset += record) {
			int32_t coordinates[2] = {};
			std::memcpy(coordinates, bytes.data() + offset, sizeof(coordinates));
			const uint1* pixels = reinterpret_cast<const uint1*>(bytes.data() + offset + sizeof(coordinates));
			placeStoredTile(layer, { coordinates[0], coordinates[1] }, storedSize, pixels);
		}
		if (storedSize != kTileSize) dropTransparentTiles(layer);
		return true;
	}

	bool saveSessionSnapshot(const std::string& directory, const SessionSnapshot& snapshot) {
		if (!saveLayerTiles(directory, snapshot.document)) return false;
		return writeBytes(std::filesystem::path(directory) / "session.txt", snapshot.serialize());
	}

	bool RecordedSession::save(const std::string& directory) const {
		const std::filesystem::path root(directory);
		bool written = saveSessionSnapshot(directory, snapshot);
		written = written && input.save((root / "input.txt").string());
		if (!recordedFrames.empty()) written = written && writeBytes(root / "recorded-frames.tsv", recordedFrames);
		return written;
	}

	std::optional<RecordedSession> RecordedSession::load(const std::string& directory) {
		const std::filesystem::path root(directory);
		std::optional<SessionSnapshot> snapshot = SessionSnapshot::parse(readBytes(root / "session.txt"));
		std::optional<InputRecording> input = InputRecording::load((root / "input.txt").string());
		if (!snapshot || !input) return std::nullopt;
		if (!loadLayerTiles(directory, snapshot->document)) return std::nullopt;

		RecordedSession session;
		session.snapshot = std::move(*snapshot);
		session.input = std::move(*input);
		if (std::filesystem::exists(root / "recorded-frames.tsv"))
			session.recordedFrames = readBytes(root / "recorded-frames.tsv");
		return session;
	}
}
