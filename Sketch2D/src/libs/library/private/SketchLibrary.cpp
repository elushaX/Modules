#include "SketchLibrary.hpp"

#include "LayerThumbnail.hpp"
#include "Session.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace tp::sketch2d {

	namespace {
		constexpr const char* kDocumentFile = "sketch.txt";
		constexpr const char* kPreviewFile = "preview.rgba";
		constexpr const char* kPreviewHeader = "sketch2d-preview 1\n";
		constexpr const char* kLegacyPreviewHeader = "leo-preview 1\n";
		constexpr const char* kOrderFile = "order.txt";

		std::filesystem::path pathOf(const std::string& directory, const char* file) {
			return std::filesystem::path(directory) / file;
		}

		void blendOver(uint1* target, const uint1* source, halnf opacity) {
			const halnf alpha = (halnf) source[3] / 255.0f * opacity;
			if (alpha <= 0) return;
			const halnf targetAlpha = (halnf) target[3] / 255.0f;
			const halnf outAlpha = alpha + targetAlpha * (1 - alpha);
			for (int4 channel = 0; channel < 3; channel++) {
				const halnf mixed =
					((halnf) source[channel] * alpha + (halnf) target[channel] * targetAlpha * (1 - alpha)) / outAlpha;
				target[channel] = (uint1) std::lround(std::clamp(mixed, 0.0f, 255.0f));
			}
			target[3] = (uint1) std::lround(outAlpha * 255.0f);
		}

		void fillBackground(std::vector<uint1>& pixels, const CanvasBackground& background) {
			const RGBA color = background.rgba();
			for (size_t offset = 0; offset < pixels.size(); offset += 4) {
				pixels[offset + 0] = (uint1) std::lround(color.r * 255.0f);
				pixels[offset + 1] = (uint1) std::lround(color.g * 255.0f);
				pixels[offset + 2] = (uint1) std::lround(color.b * 255.0f);
				pixels[offset + 3] = (uint1) std::lround(color.a * 255.0f);
			}
		}

		bool isValidNameCharacter(char character) {
			return std::isalnum((unsigned char) character) || character == ' ' || character == '-' || character == '_';
		}
	}

	SketchPreview SketchPreview::render(const Document& document) {
		document.settlePaint();
		SketchPreview preview;
		preview.rgba.assign((size_t) kSize * kSize * 4, 0);
		fillBackground(preview.rgba, document.background());

		const RectF bounds = LayerThumbnail::documentBounds(document);
		for (const auto& layer : document.layers()) {
			if (!layer->properties().visible) continue;
			const std::vector<uint1> pixels = LayerThumbnail::render(*layer, bounds, kSize);
			for (size_t offset = 0; offset < pixels.size(); offset += 4)
				blendOver(preview.rgba.data() + offset, pixels.data() + offset, layer->properties().opacity);
		}
		return preview;
	}

	SketchFile SketchFile::capture(const Document& document) {
		document.settlePaint();
		return { document.state(),
						 SketchPreview::render(document),
						 SessionView{ document.panWorld(), document.zoom(), document.rotation() } };
	}

	bool SketchFile::save(const std::string& directory) const {
		if (!saveLayerTiles(directory, document)) return false;

		std::string text = std::string(kHeader) + "\n" + serializeDocument(document);
		if (view) {
			std::ostringstream line;
			line << "view.transform " << view->pan.x << " " << view->pan.y << " " << view->zoom << " " << view->rotation
					 << "\n";
			text += line.str();
		}
		bool written = writeFileBytes(pathOf(directory, kDocumentFile).string(), text);
		if (!preview.isEmpty()) {
			const std::string bytes(reinterpret_cast<const char*>(preview.rgba.data()), preview.rgba.size());
			written = written && writeFileBytes(pathOf(directory, kPreviewFile).string(), kPreviewHeader + bytes);
		}
		return written;
	}

	std::optional<SketchFile> SketchFile::load(const std::string& directory) {
		std::istringstream input(readFileBytes(pathOf(directory, kDocumentFile).string()));
		std::string row;
		if (!std::getline(input, row) || (row != kHeader && row != kLegacyHeader)) return std::nullopt;

		SketchFile sketch;
		while (std::getline(input, row)) {
			if (row.empty()) continue;
			std::istringstream fields(row);
			std::string key;
			fields >> key;
			if (key == "view.transform") {
				SessionView view;
				fields >> view.pan.x >> view.pan.y >> view.zoom >> view.rotation;
				if (!fields.fail()) sketch.view = view;
				continue;
			}
			if (!parseDocumentLine(key, fields, sketch.document) || fields.fail()) return std::nullopt;
		}
		if (sketch.document.layers.empty()) return std::nullopt;
		if (!loadLayerTiles(directory, sketch.document)) return std::nullopt;

		if (std::optional<SketchPreview> preview = loadPreview(directory)) sketch.preview = std::move(*preview);
		return sketch;
	}

	std::optional<SketchPreview> SketchFile::loadPreview(const std::string& directory) {
		const std::string bytes = readFileBytes(pathOf(directory, kPreviewFile).string());
		const bool legacy = bytes.compare(0, std::string(kLegacyPreviewHeader).size(), kLegacyPreviewHeader) == 0;
		const size_t header = std::string(legacy ? kLegacyPreviewHeader : kPreviewHeader).size();
		if (bytes.size() <= header || (!legacy && bytes.compare(0, header, kPreviewHeader) != 0)) return std::nullopt;
		const size_t pixels = (bytes.size() - header) / 4;
		const int4 size = (int4) std::lround(std::sqrt((double) pixels));
		if (size <= 0 || (size_t) size * size * 4 != bytes.size() - header) return std::nullopt;

		SketchPreview preview;
		preview.size = size;
		preview.rgba.assign(bytes.begin() + (std::ptrdiff_t) header, bytes.end());
		return preview;
	}

	SketchLibrary::SketchLibrary(std::string root) :
		mRoot(std::move(root)) {}

	AppStorage SketchLibrary::applicationStorage() { return AppStorage::inHome("sketch2d"); }

	std::string SketchLibrary::defaultRoot() { return rootIn(applicationStorage()); }

	std::string SketchLibrary::rootIn(const AppStorage& storage) { return storage.path(kSketchesFolder); }

	std::string SketchLibrary::sanitizedName(const std::string& name) {
		std::string result;
		for (char character : name)
			result += isValidNameCharacter(character) ? character : '_';
		const size_t first = result.find_first_not_of(' ');
		const size_t last = result.find_last_not_of(' ');
		if (first == std::string::npos) return "Sketch";
		return result.substr(first, last - first + 1);
	}

	std::string SketchLibrary::sanitizedPath(const std::string& path) {
		std::string result;
		std::istringstream parts(path);
		std::string part;
		while (std::getline(parts, part, '/')) {
			if (part.empty()) continue;
			result = joined(result, sanitizedName(part));
		}
		return result;
	}

	std::string SketchLibrary::parentOf(const std::string& path) {
		const size_t slash = path.rfind('/');
		return slash == std::string::npos ? std::string() : path.substr(0, slash);
	}

	std::string SketchLibrary::nameOf(const std::string& path) {
		const size_t slash = path.rfind('/');
		return slash == std::string::npos ? path : path.substr(slash + 1);
	}

	std::string SketchLibrary::joined(const std::string& folder, const std::string& name) {
		return folder.empty() ? name : folder + "/" + name;
	}

	std::string SketchLibrary::directoryOf(const std::string& path) const {
		return (std::filesystem::path(mRoot) / sanitizedPath(path)).string();
	}

	bool SketchLibrary::contains(const std::string& path) const {
		return std::filesystem::exists(pathOf(directoryOf(path), kDocumentFile));
	}

	bool SketchLibrary::isFolder(const std::string& path) const {
		std::error_code error;
		return std::filesystem::is_directory(directoryOf(path), error) && !contains(path);
	}

	std::string SketchLibrary::unusedPath(const std::string& folder, const std::string& base) const {
		for (int4 index = 1;; index++) {
			const std::string candidate = joined(folder, base + " " + std::to_string(index));
			std::error_code error;
			if (!std::filesystem::exists(directoryOf(candidate), error)) return candidate;
		}
	}

	std::string SketchLibrary::unusedName(const std::string& base) const { return unusedPath(kScratchFolder, base); }

	void SketchLibrary::ensureScratch() const {
		std::error_code error;
		const std::filesystem::path scratch = directoryOf(kScratchFolder);
		std::filesystem::create_directories(scratch, error);
		for (const auto& item : std::filesystem::directory_iterator(mRoot, error)) {
			if (!item.is_directory() || !std::filesystem::exists(item.path() / kDocumentFile)) continue;
			const std::string name = item.path().filename().string();
			if (name.ends_with(".saving")) continue;
			const bool taken = std::filesystem::exists(scratch / name);
			const std::string target = taken ? nameOf(unusedPath(kScratchFolder, name)) : name;
			std::filesystem::rename(item.path(), scratch / target, error);
		}
	}

	std::vector<std::string> SketchLibrary::storedOrder(const std::string& folder) const {
		std::istringstream input(readFileBytes(pathOf(directoryOf(folder), kOrderFile).string()));
		std::vector<std::string> names;
		std::string name;
		while (std::getline(input, name))
			if (!name.empty()) names.push_back(name);
		return names;
	}

	std::vector<LibraryItem> SketchLibrary::children(const std::string& folder) const {
		std::vector<LibraryItem> items;
		std::error_code error;
		for (const auto& entry : std::filesystem::directory_iterator(directoryOf(folder), error)) {
			const std::string name = entry.path().filename().string();
			if (!entry.is_directory() || name.ends_with(".saving")) continue;
			const std::string path = joined(folder, name);
			if (!std::filesystem::exists(entry.path() / kDocumentFile)) {
				items.push_back({ LibraryItem::Kind::Folder, name, path, entry.path().string(), 0 });
				continue;
			}
			const auto written = std::filesystem::last_write_time(entry.path() / kDocumentFile, error);
			const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(written.time_since_epoch()).count();
			items.push_back({ LibraryItem::Kind::Sketch, name, path, entry.path().string(), (int8) seconds });
		}

		const std::vector<std::string> order = storedOrder(folder);
		const auto rank = [&order](const LibraryItem& item) {
			const auto found = std::find(order.begin(), order.end(), item.name);
			return found == order.end() ? order.size() : (size_t) (found - order.begin());
		};
		std::sort(items.begin(), items.end(), [&](const LibraryItem& left, const LibraryItem& right) {
			if (rank(left) != rank(right)) return rank(left) < rank(right);
			if (left.kind != right.kind) return left.kind == LibraryItem::Kind::Folder;
			if (left.modifiedSeconds != right.modifiedSeconds) return left.modifiedSeconds > right.modifiedSeconds;
			return left.name < right.name;
		});
		return items;
	}

	std::vector<SketchEntry> SketchLibrary::list() const {
		std::vector<SketchEntry> entries;
		std::vector<std::string> folders = { "" };
		while (!folders.empty()) {
			const std::string folder = folders.back();
			folders.pop_back();
			for (const LibraryItem& item : children(folder)) {
				if (item.kind == LibraryItem::Kind::Folder) folders.push_back(item.path);
				else entries.push_back({ item.name, item.directory, item.modifiedSeconds, item.path });
			}
		}
		std::sort(entries.begin(), entries.end(), [](const SketchEntry& left, const SketchEntry& right) {
			if (left.modifiedSeconds != right.modifiedSeconds) return left.modifiedSeconds > right.modifiedSeconds;
			return left.path < right.path;
		});
		return entries;
	}

	bool SketchLibrary::save(const std::string& path, const Document& document) const {
		return save(path, SketchFile::capture(document));
	}

	bool SketchLibrary::save(const std::string& path, const SketchFile& file) const {
		const std::filesystem::path target = directoryOf(path);
		const std::filesystem::path staging = target.string() + ".saving";
		std::error_code error;
		std::filesystem::create_directories(target.parent_path(), error);
		std::filesystem::remove_all(staging, error);
		if (!file.save(staging.string())) {
			std::filesystem::remove_all(staging, error);
			return false;
		}

		std::filesystem::remove_all(target, error);
		std::filesystem::rename(staging, target, error);
		return !error;
	}

	bool SketchLibrary::open(const std::string& path, Document& document) const {
		std::optional<SketchFile> sketch = SketchFile::load(directoryOf(path));
		if (!sketch) return false;
		document.load(sketch->document, "Open " + nameOf(sanitizedPath(path)));
		document.resetView();
		if (sketch->view) {
			document.setZoom(sketch->view->zoom);
			document.setRotation(sketch->view->rotation);
			document.setPanWorld(sketch->view->pan);
		}
		return true;
	}

	bool SketchLibrary::remove(const std::string& path) const {
		if (sanitizedPath(path).empty() || sanitizedPath(path) == kScratchFolder) return false;
		std::error_code error;
		return std::filesystem::remove_all(directoryOf(path), error) > 0 && !error;
	}

	bool SketchLibrary::createFolder(const std::string& path) const {
		std::error_code error;
		if (std::filesystem::exists(directoryOf(path), error)) return false;
		return std::filesystem::create_directories(directoryOf(path), error) && !error;
	}

	bool SketchLibrary::move(const std::string& path, const std::string& folder) const {
		const std::string source = sanitizedPath(path);
		const std::string destination = sanitizedPath(folder);
		if (source.empty() || source == kScratchFolder || parentOf(source) == destination) return false;
		if (destination == source || destination.starts_with(source + "/")) return false;
		if (!destination.empty() && !isFolder(destination)) return false;

		const std::string target = joined(destination, nameOf(source));
		std::error_code error;
		if (std::filesystem::exists(directoryOf(target), error)) return false;
		std::filesystem::rename(directoryOf(source), directoryOf(target), error);
		return !error;
	}

	bool SketchLibrary::rename(const std::string& path, const std::string& name) const {
		const std::string source = sanitizedPath(path);
		if (source.empty() || source == kScratchFolder) return false;
		const std::string target = joined(parentOf(source), sanitizedName(name));
		if (target == source) return true;
		std::error_code error;
		if (std::filesystem::exists(directoryOf(target), error)) return false;
		std::filesystem::rename(directoryOf(source), directoryOf(target), error);
		return !error;
	}

	bool SketchLibrary::reorder(const std::string& folder, const std::vector<std::string>& names) const {
		std::string text;
		for (const std::string& name : names)
			text += name + "\n";
		return writeFileBytes(pathOf(directoryOf(folder), kOrderFile).string(), text);
	}
}
