#pragma once

#include "Layout.hpp"

#include <optional>

namespace tp::ui {

	class StackLayout : public Layout {
	protected:
		[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override;
		void arrangeChildren(LayoutContext& context, const RectF& contentArea) override;
	};

	class FreeLayout : public Layout {
	protected:
		[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override;
		void arrangeChildren(LayoutContext& context, const RectF& contentArea) override;
	};

	class GridLayout : public Layout {
	public:
		explicit GridLayout(ualni columns, halnf gap = 0);

		void setColumns(ualni columns);
		void setGap(halnf gap);
		void setCellSize(std::optional<Vec2F> cellSize);
		[[nodiscard]] const std::optional<Vec2F>& cellSize() const { return mCellSize; }

	protected:
		[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override;
		void arrangeChildren(LayoutContext& context, const RectF& contentArea) override;

	private:
		[[nodiscard]] ualni rowCount(const LayoutContext& context) const;
		[[nodiscard]] halnf columnWidth(halnf availableWidth) const;
		[[nodiscard]] halnf rowHeight(const LayoutContext& context, ualni row, halnf cellWidth) const;

		ualni mColumns;
		halnf mGap;
		std::optional<Vec2F> mCellSize;
	};
}
