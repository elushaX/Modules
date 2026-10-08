#pragma once

#include "Geometry.hpp"
#include "Color.hpp"

#include <string>
#include <vector>

namespace tp::ui {

	enum class TextAlign { Start, Center, End };
	enum class TextDirection { BottomToTop, TopToBottom };

	enum class FontFace { Regular, Bold, Italic, BoldItalic, Monospace };

	struct TextStyle {
		halnf fontSize = 13;
		FontFace face = FontFace::Regular;
	};

	struct ImageId {
		ualni value = 0;
		[[nodiscard]] bool isValid() const { return value != 0; }
	};

	struct LoadedImage {
		ImageId id;
		Vec2F size = { 0, 0 };

		[[nodiscard]] bool isValid() const { return id.isValid() && size.x > 0 && size.y > 0; }
	};

	class ImageLoader {
	public:
		virtual ~ImageLoader() = default;
		[[nodiscard]] virtual LoadedImage load(const std::string& path) = 0;
	};

	class TextMetrics {
	public:
		virtual ~TextMetrics() = default;
		[[nodiscard]] halnf textWidth(const std::string& text, halnf fontSize) const {
			return styledTextWidth(text, { fontSize, FontFace::Regular });
		}
		[[nodiscard]] virtual halnf styledTextWidth(const std::string& text, const TextStyle& style) const = 0;
		[[nodiscard]] virtual std::vector<halnf> prefixWidths(const std::string& text, const TextStyle& style) const;
	};

	class Painter : public TextMetrics {
	public:
		virtual void pushTranslation(Vec2F offset) = 0;
		virtual void popTranslation() = 0;
		virtual void pushClip(const RectF& rect) = 0;
		virtual void popClip() = 0;
		[[nodiscard]] virtual bool isClippedOut(const RectF& rect) const { return false; }

		void fillRect(const RectF& rect, const RGBA& color, halnf cornerRadius = 0) {
			drawFilledRect(rect, color, cornerRadius);
		}
		void strokeRect(const RectF& rect, const RGBA& color, halnf cornerRadius = 0, halnf width = 1) {
			drawStrokedRect(rect, color, cornerRadius, width);
		}
		void strokeCircle(Vec2F center, halnf radius, const RGBA& color, halnf width = 1) {
			drawStrokedCircle(center, radius, color, width);
		}
		void image(const RectF& rect, ImageId image, halnf cornerRadius = 0) { drawImage(rect, image, cornerRadius); }

		virtual void fillCircle(Vec2F center, halnf radius, const RGBA& color) = 0;
		virtual void line(Vec2F from, Vec2F to, const RGBA& color, halnf thickness) = 0;
		virtual void polyline(const std::vector<Vec2F>& points, const RGBA& color, halnf thickness) = 0;
		virtual void fillPolygon(const std::vector<Vec2F>& points, const RGBA& color) = 0;
		void text(const std::string& text, const RectF& box, halnf fontSize, TextAlign align, const RGBA& color) {
			styledText(text, box, { fontSize, FontFace::Regular }, align, color);
		}
		virtual void styledText(
			const std::string& text, const RectF& box, const TextStyle& style, TextAlign align, const RGBA& color
		) = 0;
		virtual void verticalText(
			const std::string& text, const RectF& box, halnf fontSize, TextDirection direction, const RGBA& color
		) = 0;
		virtual void colorWheel(const RectF& rect, const HSV& selected, halnf ringThickness) = 0;
		virtual void
		fillGradientRect(const RectF& rect, const RGBA& from, const RGBA& to, bool horizontal, halnf cornerRadius = 0) = 0;

	protected:
		virtual void drawFilledRect(const RectF& rect, const RGBA& color, halnf cornerRadius) = 0;
		virtual void drawStrokedRect(const RectF& rect, const RGBA& color, halnf cornerRadius, halnf width) = 0;
		virtual void drawStrokedCircle(Vec2F center, halnf radius, const RGBA& color, halnf width) = 0;
		virtual void drawImage(const RectF& rect, ImageId image, halnf cornerRadius) = 0;
	};

	class ScopedTranslation {
	public:
		ScopedTranslation(Painter& painter, Vec2F offset) :
			mPainter(painter) {
			mPainter.pushTranslation(offset);
		}

		~ScopedTranslation() { mPainter.popTranslation(); }

		ScopedTranslation(const ScopedTranslation&) = delete;
		ScopedTranslation& operator=(const ScopedTranslation&) = delete;

	private:
		Painter& mPainter;
	};

	class ScopedClip {
	public:
		ScopedClip(Painter& painter, const RectF& rect) :
			mPainter(painter) {
			mPainter.pushClip(rect);
		}

		~ScopedClip() { mPainter.popClip(); }

		ScopedClip(const ScopedClip&) = delete;
		ScopedClip& operator=(const ScopedClip&) = delete;

	private:
		Painter& mPainter;
	};
}
