#pragma once

#include "Containers.hpp"
#include "Control.hpp"

#include <functional>
#include <string>

namespace tp::ui {

	class CollapsibleHeader : public Control {
	public:
		explicit CollapsibleHeader(std::string title);

		void setTitle(std::string title);
		[[nodiscard]] const std::string& title() const { return mTitle; }
		void setSummary(std::string summary);
		[[nodiscard]] const std::string& summary() const { return mSummary; }
		void setExpanded(bool expanded, bool animated = true);
		[[nodiscard]] halnf chevronTurn() const { return mTurn.value(); }
		void setAction(Widget* action);
		[[nodiscard]] Widget* action() const { return mAction; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		class HeaderLayout;
		friend class HeaderLayout;

		[[nodiscard]] halnf actionWidth() const;

		std::string mTitle;
		std::string mSummary;
		Widget* mAction = nullptr;
		AnimatedFloat mTurn{ 0 };
	};

	class Collapsible : public Widget {
	public:
		explicit Collapsible(std::string title = {});

		void setExpanded(bool expanded, bool animated = true);
		[[nodiscard]] bool isExpanded() const { return mExpanded; }
		void toggle() { setExpanded(!mExpanded); }

		void setOnToggled(std::function<void(bool)> onToggled) { mOnToggled = std::move(onToggled); }

		[[nodiscard]] Widget& body() { return mBody; }
		[[nodiscard]] CollapsibleHeader& header() { return mHeader; }
		void setHeaderAction(Widget* action) { mHeader.setAction(action); }
		[[nodiscard]] halnf openAmount() const { return mOpen.value(); }
		[[nodiscard]] bool isBodyShown() const { return mReveal.isVisible(); }
		[[nodiscard]] bool isTransitioning() const override { return mOpen.isAnimating(); }

	protected:
		void onTick(halnf deltaMs) override;
		void finishTransitions() override;

	private:
		class SectionLayout;
		friend class SectionLayout;

		CollapsibleHeader mHeader;
		Widget mReveal;
		Widget mBody;
		bool mExpanded = false;
		AnimatedFloat mOpen{ 0, Motion::smooth() };
		std::function<void(bool)> mOnToggled;
	};
}
