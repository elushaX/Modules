#pragma once

#include "Control.hpp"

#include <optional>
#include <string>

namespace tp::ui {

	class Button : public Control {
	public:
		enum class Style { Normal, Accent, Flat, Tab };
		enum class IconPlacement { Leading, Above };

		explicit Button(std::string text = {}, std::function<void()> onClick = {});

		void setText(std::string text);
		void setIconSize(halnf size) {
			mIconSize = size;
			requestLayout();
		}
		[[nodiscard]] halnf iconSize() const;
		[[nodiscard]] halnf cornerRadius() const;
		void setFill(std::optional<RGBA> fill);
		void setCornerRadius(halnf radius) {
			mCornerRadius = radius;
			requestPaint();
		}
		[[nodiscard]] const std::string& text() const { return mText; }
		void setFontFace(FontFace face);
		[[nodiscard]] FontFace fontFace() const { return mFontFace; }

		void setIcon(ImageId icon);
		[[nodiscard]] ImageId icon() const { return mIcon; }
		void setIconPlacement(IconPlacement placement);
		void setStyle(Style style);
		void setSelected(bool selected);
		[[nodiscard]] bool isSelected() const { return mSelected; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;

		[[nodiscard]] virtual bool hasIcon() const { return mIcon.isValid(); }
		virtual void paintIcon(Painter& painter, const RectF& box);
		[[nodiscard]] virtual RGBA labelColor() const;

	private:
		struct ContentBoxes {
			RectF icon;
			RectF text;
		};

		[[nodiscard]] ContentBoxes contentBoxes() const;
		[[nodiscard]] halnf labelFontSize() const;
		[[nodiscard]] TextStyle labelStyle() const { return { labelFontSize(), mFontFace }; }

		std::string mText;
		ImageId mIcon;
		IconPlacement mIconPlacement = IconPlacement::Leading;
		Style mStyle = Style::Normal;
		FontFace mFontFace = FontFace::Regular;
		bool mSelected = false;
		halnf mIconSize = 0;
		halnf mCornerRadius = -1;
		std::optional<RGBA> mFill;
	};

	class GlyphButton : public Button {
	public:
		enum class Glyph { Options, Hide, Previous, Next, Up };

		explicit GlyphButton(Glyph glyph);

		[[nodiscard]] Glyph glyph() const { return mGlyph; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		void paintChevron(Painter& painter, halnf direction, const RGBA& color) const;
		void paintUpChevron(Painter& painter, const RGBA& color) const;

		Glyph mGlyph;
	};
}
