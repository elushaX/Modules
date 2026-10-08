#pragma once

#include "Widget.hpp"
#include "Painter.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	class Label : public Widget {
	public:
		enum class Tone { Normal, Muted, Warning };

		explicit Label(std::string text = {});

		void setText(std::string text);
		[[nodiscard]] const std::string& text() const { return mText; }

		void setAlignment(TextAlign alignment);
		[[nodiscard]] TextAlign alignment() const { return mAlignment; }
		void setFontSize(halnf fontSize);
		void setTone(Tone tone);
		void setWrap(bool wrap);
		[[nodiscard]] bool wraps() const { return mWrap; }
		void setMaximumLines(std::optional<size_t> lines);
		[[nodiscard]] std::optional<size_t> maximumLines() const { return mMaximumLines; }
		void setElidesToHeight(bool elides);
		[[nodiscard]] bool elidesToHeight() const { return mElidesToHeight; }
		[[nodiscard]] std::vector<std::string> visibleLines() const;
		[[nodiscard]] const std::vector<std::string>& linesFor(halnf width) const;
		void setReservedText(std::string text);
		[[nodiscard]] const std::string& reservedText() const { return mReservedText; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;

	public:
		[[nodiscard]] halnf effectiveFontSize() const;

	private:
		struct TextLayoutKey {
			halnf width = -1;
			halnf fontSize = 0;
			const TextMetrics* metrics = nullptr;

			bool operator==(const TextLayoutKey&) const = default;
		};

		struct LaidOutLines {
			std::optional<TextLayoutKey> key;
			std::vector<std::string> lines;
		};

		struct WidestLine {
			std::optional<TextLayoutKey> key;
			halnf width = 0;
		};

		[[nodiscard]] halnf lineHeight() const;
		void paintLines(Painter& painter, const std::vector<std::string>& lines, const RGBA& color) const;
		[[nodiscard]] std::vector<std::string> layOutLines(halnf width) const;
		[[nodiscard]] std::vector<std::string> limitedLines(std::vector<std::string> lines, halnf width) const;
		[[nodiscard]] std::vector<std::string> elidedLines(std::vector<std::string> lines, size_t count, halnf width) const;
		[[nodiscard]] size_t linesFittingHeight() const;
		[[nodiscard]] halnf widestLine() const;
		[[nodiscard]] TextLayoutKey layoutKey(halnf width) const;
		void invalidateLines();

		std::string mText;
		std::string mReservedText;
		std::vector<std::string> mLines;
		TextAlign mAlignment = TextAlign::Start;
		halnf mFontSize = 0;
		Tone mTone = Tone::Normal;
		bool mWrap = false;
		std::optional<size_t> mMaximumLines;
		bool mElidesToHeight = false;
		mutable LaidOutLines mLaidOut;
		mutable WidestLine mWidestLine;
	};

	[[nodiscard]] std::vector<std::string> wrapText(
		const std::string& text,
		halnf width,
		const std::function<halnf(const std::string&)>& measure,
		std::optional<size_t> maximumLines = std::nullopt
	);

	class TooltipBubble : public Widget {
	public:
		TooltipBubble();

		void setText(std::string text);
		[[nodiscard]] const std::string& text() const { return mText; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;

	private:
		std::string mText;
	};
}
