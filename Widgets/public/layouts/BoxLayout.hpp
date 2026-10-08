#pragma once

#include "Layout.hpp"

namespace tp::ui {

	enum class CrossAlign { Stretch, Start, Center, End };
	enum class MainAlign { Start, Center, End, SpaceBetween };

	class BoxLayout : public Layout {
	public:
		explicit BoxLayout(Axis axis, halnf gap = 0);

		void setGap(halnf gap);
		void setCrossAlign(CrossAlign align);
		void setMainAlign(MainAlign align);

		[[nodiscard]] Axis axis() const { return mAxis; }
		[[nodiscard]] halnf gap() const { return mGap; }

	protected:
		[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override;
		void arrangeChildren(LayoutContext& context, const RectF& contentArea) override;

	private:
		[[nodiscard]] Extent measureMainAxis(const LayoutContext& context, halnf crossLength) const;
		[[nodiscard]] Extent measureCrossAxis(const LayoutContext& context, halnf mainLength) const;
		[[nodiscard]] halnf totalGap(const LayoutContext& context) const;
		[[nodiscard]] halnf crossLengthFor(const Extent& extent, halnf available) const;
		[[nodiscard]] halnf crossStartFor(halnf length, halnf available) const;

		Axis mAxis;
		halnf mGap;
		CrossAlign mCrossAlign = CrossAlign::Stretch;
		MainAlign mMainAlign = MainAlign::Start;
	};

	class RowLayout : public BoxLayout {
	public:
		explicit RowLayout(halnf gap = 0) :
			BoxLayout(Axis::Horizontal, gap) {}
	};

	class ColumnLayout : public BoxLayout {
	public:
		explicit ColumnLayout(halnf gap = 0) :
			BoxLayout(Axis::Vertical, gap) {}
	};
}
