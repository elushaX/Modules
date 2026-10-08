#pragma once

#include "ViewSettings.hpp"
#include "Document.hpp"
#include "InputRecording.hpp"
#include "SessionRecording.hpp"

#include <optional>
#include <sstream>
#include <string>

namespace tp::sketch2d {

	struct SessionView {
		Vec2F pan = { 0, 0 };
		halnf zoom = 1;
		halnf rotation = 0;
	};

	struct SessionSnapshot {
		static constexpr const char* kHeader = "sketch2d-session 1";
		static constexpr const char* kLegacyHeader = "leo-session 1";

		SessionEnvironment environment;
		SessionView view;
		Brush brush;
		ViewSettings settings;
		DocumentState document;

		[[nodiscard]] static SessionSnapshot
		capture(const Document& document, const Brush& brush, const ViewSettings& settings, SessionEnvironment environment);
		void applyTo(Document& document, Brush& brush, ViewSettings& settings) const;

		[[nodiscard]] std::string serialize() const;
		[[nodiscard]] static std::optional<SessionSnapshot> parse(const std::string& text);
	};

	[[nodiscard]] std::string serializeLayerTiles(const Layer::State& layer);
	[[nodiscard]] bool parseLayerTiles(const std::string& bytes, Layer::State& layer);

	[[nodiscard]] std::string serializeDocument(const DocumentState& document);
	[[nodiscard]] bool parseDocumentLine(const std::string& key, std::istringstream& fields, DocumentState& document);
	bool saveLayerTiles(const std::string& directory, const DocumentState& document);
	bool loadLayerTiles(const std::string& directory, DocumentState& document);
	[[nodiscard]] std::string readFileBytes(const std::string& path);
	bool writeFileBytes(const std::string& path, const std::string& bytes);

	struct RecordedSession {
		SessionSnapshot snapshot;
		InputRecording input;
		std::string recordedFrames;

		bool save(const std::string& directory) const;
		[[nodiscard]] static std::optional<RecordedSession> load(const std::string& directory);
	};

	bool saveSessionSnapshot(const std::string& directory, const SessionSnapshot& snapshot);
}
