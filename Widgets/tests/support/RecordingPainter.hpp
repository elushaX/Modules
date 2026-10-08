#pragma once

#include "Painter.hpp"

#include <string>
#include <vector>

namespace tp::ui::testing {

	struct PaintCommand {
		enum class Kind {
			FillRect,
			StrokeRect,
			Circle,
			StrokeCircle,
			Line,
			Polyline,
			Polygon,
			Text,
			Image,
			ColorWheel,
			Gradient
		};

		Kind kind;
		RectF rect;
		RGBA color;
		halnf cornerRadius = 0;
		std::string text;
		RectF clip;
		std::vector<Vec2F> points = {};
		RGBA endColor = {};
		FontFace face = FontFace::Regular;
		halnf fontSize = 0;
	};

	class RecordingPainter : public Painter {
	public:
		void pushTranslation(Vec2F offset) override;
		void popTranslation() override;
		void pushClip(const RectF& rect) override;
		void popClip() override;

		void fillCircle(Vec2F center, halnf radius, const RGBA& color) override;
		void line(Vec2F from, Vec2F to, const RGBA& color, halnf thickness) override;
		void polyline(const std::vector<Vec2F>& points, const RGBA& color, halnf thickness) override;
		void fillPolygon(const std::vector<Vec2F>& points, const RGBA& color) override;
		void styledText(
			const std::string& text, const RectF& box, const TextStyle& style, TextAlign align, const RGBA& color
		) override;
		void verticalText(
			const std::string& text, const RectF& box, halnf fontSize, TextDirection direction, const RGBA& color
		) override;
		void colorWheel(const RectF& rect, const HSV& selected, halnf ringThickness) override;
		void fillGradientRect(
			const RectF& rect, const RGBA& from, const RGBA& to, bool horizontal, halnf cornerRadius = 0
		) override;

		[[nodiscard]] halnf styledTextWidth(const std::string& text, const TextStyle& style) const override;
		[[nodiscard]] bool isClippedOut(const RectF& rect) const override;

		[[nodiscard]] const std::vector<PaintCommand>& commands() const { return mCommands; }
		[[nodiscard]] const PaintCommand* findText(const std::string& text) const;
		[[nodiscard]] bool hasText(const std::string& text) const { return findText(text) != nullptr; }
		[[nodiscard]] bool isBalanced() const { return mOrigins.size() == 1 && mClips.empty(); }
		void clear() { mCommands.clear(); }

	protected:
		void drawFilledRect(const RectF& rect, const RGBA& color, halnf cornerRadius) override;
		void drawStrokedRect(const RectF& rect, const RGBA& color, halnf cornerRadius, halnf width) override;
		void drawStrokedCircle(Vec2F center, halnf radius, const RGBA& color, halnf width) override;
		void drawImage(const RectF& rect, ImageId image, halnf cornerRadius) override;

	private:
		void record(
			PaintCommand::Kind kind,
			const RectF& localRect,
			const RGBA& color,
			const std::string& text = {},
			halnf cornerRadius = 0
		);

		std::vector<Vec2F> mOrigins = { { 0, 0 } };
		std::vector<RectF> mClips;
		std::vector<PaintCommand> mCommands;
	};

	class MonospaceTextMetrics : public TextMetrics {
	public:
		static constexpr halnf kCharacterWidthFactor = 0.5f;

		[[nodiscard]] halnf styledTextWidth(const std::string& text, const TextStyle& style) const override {
			mMeasurements++;
			return (halnf) text.size() * style.fontSize * kCharacterWidthFactor;
		}

		[[nodiscard]] size_t measurements() const { return mMeasurements; }

	private:
		mutable size_t mMeasurements = 0;
	};
}
