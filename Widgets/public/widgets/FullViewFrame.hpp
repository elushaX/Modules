#pragma once

#include "Animation.hpp"
#include "Widget.hpp"

#include <functional>
#include <optional>

namespace tp::ui {

	// The most detailed level of an item in a collection view (a tree card, a kanban card): the card itself grown to
	// fill the host. It paints the card surface with its accent strip; it has no header of its own, so the content
	// supplies any Back affordance and calls requestBack(). The host places it with rectFor(), which grows from the
	// item's rect at progress 0 to the host's whole area at progress 1, and springs between progress targets on its
	// own. Until the midpoint the frame shows the preview (the card's own face, so the card visibly grows); past it,
	// the full content. The frame takes keyboard
	// focus itself (hosts give it focus when it opens, a click on its background does too); Escape asks to go back only
	// while the frame itself has focus, so a focused child such as a text editor keeps its Escape.
	class FullViewFrame : public Widget {
	public:
		FullViewFrame();
		~FullViewFrame() override;

		void setContent(Widget* content);
		[[nodiscard]] Widget* content() const { return mContent; }
		void setPreview(Widget* preview);
		[[nodiscard]] Widget* preview() const { return mPreview; }
		void setAccent(std::optional<RGBA> accent);
		[[nodiscard]] const std::optional<RGBA>& accent() const { return mAccent; }
		void setOnBack(std::function<void()> callback) { mOnBack = std::move(callback); }
		void requestBack();

		void showProgress(halnf progress, bool animated = true);
		[[nodiscard]] halnf progress() const { return mProgress.value(); }
		[[nodiscard]] halnf targetProgress() const { return mProgress.target(); }
		[[nodiscard]] bool isAnimating() const { return mProgress.isAnimating(); }
		[[nodiscard]] bool isShown() const { return progress() > kHiddenProgress || targetProgress() > 0; }
		[[nodiscard]] bool isContentShown() const;

		[[nodiscard]] RectF rectFor(const RectF& item, const RectF& area) const;

		static constexpr halnf kHiddenProgress = 0.001f;
		static constexpr halnf kContentShownProgress = 0.5f;
		static constexpr halnf kContentPadding = 8;
		static constexpr halnf kMargin = 8;
		static constexpr halnf kAccentWidth = 4;
		static constexpr halnf kAccentInset = 5;

	protected:
		void paint(Painter& painter) override;
		void onTick(halnf deltaMs) override;
		void onEvent(Event& event) override;
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return isShown(); }

	private:
		void progressChanged();
		void showStage();

		Widget* mContent = nullptr;
		Widget mContentSlot;
		Widget mPreviewSlot;
		Widget* mPreview = nullptr;
		std::optional<RGBA> mAccent;
		AnimatedFloat mProgress{ 0, Motion::smooth(), 0.0005f };
		std::function<void()> mOnBack;
	};
}
