#pragma once

#include "Painter.hpp"
#include "Widget.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	struct TextSpan {
		std::string text;
		FontFace face = FontFace::Regular;
		std::optional<RGBA> color;
		std::optional<RGBA> background;
		std::string link;
		bool strikethrough = false;
		bool underline = false;
		bool lineBreak = false;
		size_t sourceOffset = 0;
	};

	struct PlacedSpanPiece {
		size_t span = 0;
		std::string text;
		RectF box;
		halnf inkWidth = 0;
	};

	class RichText : public Widget {
	public:
		RichText();

		void setSpans(std::vector<TextSpan> spans);
		[[nodiscard]] const std::vector<TextSpan>& spans() const { return mSpans; }
		void setFontSize(halnf fontSize);
		[[nodiscard]] halnf fontSize() const;
		void setLineSpacing(halnf factor);
		void setBaseColor(std::optional<RGBA> color);
		void setAlignment(TextAlign alignment);
		void setPreservesWhitespace(bool preserves);

		[[nodiscard]] std::string plainText() const;
		[[nodiscard]] const std::vector<PlacedSpanPiece>& piecesFor(halnf width) const;
		[[nodiscard]] std::optional<size_t> spanAt(Vec2F localPoint) const;

		void setOnLinkActivated(std::function<void(const std::string&)> callback) { mOnLink = std::move(callback); }
		void setOnSourceActivated(std::function<void(size_t)> callback) { mOnSource = std::move(callback); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		void onEvent(Event& event) override;
		void onHoverChanged() override;

	private:
		struct MeasuredWord {
			std::string text;
			halnf width = 0;
			halnf visibleWidth = 0;
			halnf inkWidth = 0;
		};

		struct MeasureKey {
			halnf fontSize = 0;
			const TextMetrics* metrics = nullptr;

			bool operator==(const MeasureKey&) const = default;
		};

		[[nodiscard]] MeasureKey measureKey() const;
		[[nodiscard]] const std::vector<std::vector<MeasuredWord>>& measuredWords() const;
		void invalidateMeasurements();
		[[nodiscard]] halnf lineHeight() const;
		[[nodiscard]] TextStyle styleOf(const TextSpan& span) const;
		[[nodiscard]] halnf widthOf(const std::string& text, const TextSpan& span) const;
		[[nodiscard]] halnf widestUnbreakable() const;
		void layoutFor(halnf width) const;
		void alignLines() const;
		[[nodiscard]] halnf inkWidthOf(const std::string& text, const TextSpan& span) const;
		void updateHoverFrom(Vec2F localPoint);
		void paintPiece(Painter& painter, const PlacedSpanPiece& piece) const;

		std::vector<TextSpan> mSpans;
		halnf mFontSize = 0;
		halnf mLineSpacing = 1;
		std::optional<RGBA> mBaseColor;
		TextAlign mAlignment = TextAlign::Start;
		bool mPreservesWhitespace = false;
		std::optional<size_t> mHoveredSpan;

		mutable std::vector<std::vector<MeasuredWord>> mWords;
		mutable std::optional<MeasureKey> mWordsKey;
		mutable std::vector<PlacedSpanPiece> mPieces;
		mutable std::optional<MeasureKey> mPiecesKey;
		mutable halnf mLaidOutWidth = -1;
		mutable size_t mLineCount = 0;

		std::function<void(const std::string&)> mOnLink;
		std::function<void(size_t)> mOnSource;
	};
}
