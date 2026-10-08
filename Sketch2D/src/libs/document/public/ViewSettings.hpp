#pragma once

#include "BrushCursor.hpp"
#include "ChannelView.hpp"
#include "Document.hpp"

namespace tp::sketch2d {

	enum class PickerStyle { Cursor, Ring, Balloon, Magnifier };

	enum class CanvasTool {
		Paint,
		Picker,
		Hand,
		Zoom,
		Rotate,
		Bucket,
		Gradient,
		Move,
		Transform,
		Crop,
		Marquee,
		Lasso,
		Wand
	};

	enum class MarqueeShape { Rectangle, Ellipse };
	enum class LassoShape { Freehand, Polygon };

	struct SelectionSettings {
		MarqueeShape marquee = MarqueeShape::Rectangle;
		LassoShape lasso = LassoShape::Freehand;
		bool clickToDeselect = true;
		bool autoFill = false;
		int4 adjustPixels = 4;
		BucketSettings wand = { 0.1f, false, true, 0 };
	};

	struct TransformSettings {
		bool proportional = false;
	};

	struct PickerSettings {
		bool sampleAllLayers = true;
		bool sampleOpacity = false;
		PickerStyle style = PickerStyle::Ring;
	};

	enum class LineMode { Normal, Ruler, Ellipse, Perspective };

	enum class PerspectiveType { OnePoint, TwoPoint, ThreePoint, Isometric, Radial };

	struct PerspectiveSettings {
		PerspectiveType type = PerspectiveType::TwoPoint;
		Vec2F points[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
		halnf isometricDegrees = 30.0f;
		bool locked = false;
		bool placed = false;

		[[nodiscard]] int4 pointCount() const;
		[[nodiscard]] std::vector<Vec2F> directions(Vec2F from) const;
		[[nodiscard]] Vec2F snap(Vec2F from, Vec2F to) const;
	};

	struct SymmetrySettings {
		bool enabled = false;
		bool visible = true;
		bool locked = false;
		Vec2F center = { 0, 0 };
		halnf angle = 1.5707963f;

		[[nodiscard]] Vec2F mirror(Vec2F point) const;
	};

	struct LineSettings {
		bool holdForLine = false;
		halnf holdSeconds = 0.5f;
		LineMode mode = LineMode::Normal;
	};

	struct ViewSettings {
		HueColor boardColor = { 0.0f, 0.0f, 0.6f };
		bool canvasShadow = true;
		halnf zoomInLimit = -6.0f;
		halnf zoomOutLimit = 1.0f;
		bool showChannel = false;
		bool useBrushStabilizer = true;
		halnf globalStabilizer = Brush::kDefaultStabilizer;
		halnf viewStiffness = 0.5f;
		bool lockView = false;
		ViewChannel channel = ViewChannel::Red;
		BrushCursorSettings cursor;
		PickerSettings picker;
		BucketSettings bucket;
		LineSettings line;
		SymmetrySettings symmetry;
		PerspectiveSettings perspective;
		GradientSettings gradient;
		MoveSettings move;
		TransformSettings transform;
		SelectionSettings selection;
	};
}
