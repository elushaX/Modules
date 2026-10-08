#pragma once

#include "Graphics.hpp"
#include "Painter.hpp"

#include <vector>

namespace tp::ui {

	class CanvasPainter : public Painter {
	public:
		explicit CanvasPainter(Canvas& canvas);

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
		[[nodiscard]] std::vector<halnf> prefixWidths(const std::string& text, const TextStyle& style) const override;
		[[nodiscard]] bool isClippedOut(const RectF& rect) const override;

	protected:
		void drawFilledRect(const RectF& rect, const RGBA& color, halnf cornerRadius) override;
		void drawStrokedRect(const RectF& rect, const RGBA& color, halnf cornerRadius, halnf width) override;
		void drawStrokedCircle(Vec2F center, halnf radius, const RGBA& color, halnf width) override;
		void drawImage(const RectF& rect, ImageId image, halnf cornerRadius) override;

	private:
		[[nodiscard]] Vec2F origin() const { return mOrigins.back(); }

		Canvas& mCanvas;
		std::vector<Vec2F> mOrigins;
	};
}
