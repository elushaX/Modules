#pragma once

#include "AppStorage.hpp"

#include "Document.hpp"
#include "Session.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tp::sketch2d {

	struct SketchPreview {
		static constexpr int4 kSize = 256;
		int4 size = kSize;

		std::vector<uint1> rgba;

		[[nodiscard]] bool isEmpty() const { return rgba.empty(); }
		[[nodiscard]] static SketchPreview render(const Document& document);
	};

	struct SketchFile {
		static constexpr const char* kHeader = "sketch2d-sketch 1";
		static constexpr const char* kLegacyHeader = "leo-sketch 1";

		DocumentState document;
		SketchPreview preview;
		std::optional<SessionView> view;

		[[nodiscard]] static SketchFile capture(const Document& document);
		bool save(const std::string& directory) const;
		[[nodiscard]] static std::optional<SketchFile> load(const std::string& directory);
		[[nodiscard]] static std::optional<SketchPreview> loadPreview(const std::string& directory);
	};

	struct SketchEntry {
		std::string name;
		std::string directory;
		int8 modifiedSeconds = 0;
		std::string path;
	};

	struct LibraryItem {
		enum class Kind { Folder, Sketch };

		Kind kind = Kind::Sketch;
		std::string name;
		std::string path;
		std::string directory;
		int8 modifiedSeconds = 0;
	};

	class SketchLibrary {
	public:
		static constexpr const char* kScratchFolder = "Scratch";
		static constexpr const char* kSketchesFolder = "sketches";

		explicit SketchLibrary(std::string root);

		[[nodiscard]] const std::string& root() const { return mRoot; }
		[[nodiscard]] std::vector<SketchEntry> list() const;
		[[nodiscard]] std::vector<LibraryItem> children(const std::string& folder) const;
		[[nodiscard]] std::string directoryOf(const std::string& path) const;
		[[nodiscard]] bool contains(const std::string& path) const;
		[[nodiscard]] bool isFolder(const std::string& path) const;
		[[nodiscard]] std::string unusedName(const std::string& base = "Sketch") const;
		[[nodiscard]] std::string unusedPath(const std::string& folder, const std::string& base) const;

		bool save(const std::string& path, const Document& document) const;
		bool save(const std::string& path, const SketchFile& file) const;
		bool open(const std::string& path, Document& document) const;
		bool remove(const std::string& path) const;
		bool createFolder(const std::string& path) const;
		bool move(const std::string& path, const std::string& folder) const;
		bool rename(const std::string& path, const std::string& name) const;
		bool reorder(const std::string& folder, const std::vector<std::string>& names) const;
		void ensureScratch() const;

		[[nodiscard]] static AppStorage applicationStorage();
		[[nodiscard]] static std::string defaultRoot();
		[[nodiscard]] static std::string rootIn(const AppStorage& storage);
		[[nodiscard]] static std::string sanitizedName(const std::string& name);
		[[nodiscard]] static std::string sanitizedPath(const std::string& path);
		[[nodiscard]] static std::string parentOf(const std::string& path);
		[[nodiscard]] static std::string nameOf(const std::string& path);
		[[nodiscard]] static std::string joined(const std::string& folder, const std::string& name);

	private:
		[[nodiscard]] std::vector<std::string> storedOrder(const std::string& folder) const;

		std::string mRoot;
	};
}
