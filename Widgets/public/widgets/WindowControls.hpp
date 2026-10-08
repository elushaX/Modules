#pragma once

#include "Button.hpp"
#include "Widget.hpp"

namespace tp::ui {

	class WindowButton : public Button {
	public:
		enum class Kind { Minimize, Maximize, Close };

		explicit WindowButton(Kind kind);

		[[nodiscard]] Kind kind() const { return mKind; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;
		void onActivated() override;

	private:
		void paintGlyph(Painter& painter, Vec2F center, const RGBA& color) const;

		Kind mKind;
	};

	class WindowControls : public Widget {
	public:
		WindowControls();

		[[nodiscard]] WindowButton& minimizeButton() { return mMinimize; }
		[[nodiscard]] WindowButton& maximizeButton() { return mMaximize; }
		[[nodiscard]] WindowButton& closeButton() { return mClose; }

	protected:
		void onAttached() override;
		void onWindowHostChanged() override;

	private:
		void followWindowHost();

		WindowButton mMinimize{ WindowButton::Kind::Minimize };
		WindowButton mMaximize{ WindowButton::Kind::Maximize };
		WindowButton mClose{ WindowButton::Kind::Close };
	};
}
