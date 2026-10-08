#pragma once

#include <string>

namespace tp {

	struct ClipboardImage {
		std::string mimeType;
		std::string bytes;

		[[nodiscard]] bool isEmpty() const { return bytes.empty(); }
	};

	enum class CursorShape {
		Default,
		Pointer,
		Text,
		Move,
		Grab,
		Grabbing,
		ResizeHorizontal,
		ResizeVertical,
		ResizeNorthWestSouthEast,
		ResizeNorthEastSouthWest,
		NotAllowed,
		Crosshair,
		Hidden
	};

	struct WindowIdentity {
		std::string appId;
		std::string title;

		[[nodiscard]] static WindowIdentity generic();
	};

	enum class WindowEdge { None, Top, Bottom, Left, Right, TopLeft, TopRight, BottomLeft, BottomRight };

	CursorShape cursorForEdge(WindowEdge edge);
}
