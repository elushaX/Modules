#pragma once

#include "Containers.hpp"
#include "Control.hpp"
#include "Menus.hpp"
#include "Views.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace tp::ui {

	class ColorCell : public Control {
	public:
		enum class Kind { Swatch, Add };

		static constexpr halnf kSize = 28;

		explicit ColorCell(Kind kind, RGBA color = {});

		[[nodiscard]] Kind kind() const { return mKind; }
		[[nodiscard]] const RGBA& color() const { return mColor; }
		void setSelected(bool selected);
		[[nodiscard]] bool isSelected() const { return mSelected; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;

	private:
		Kind mKind;
		RGBA mColor;
		bool mSelected = false;
	};

	class ColorPickerButton : public PopupButton {
	public:
		static constexpr ualni kColumns = 4;

		ColorPickerButton();

		void setPalette(std::vector<RGBA> colors);
		[[nodiscard]] const std::vector<RGBA>& palette() const { return mPalette; }
		void setFallbackColor(const RGBA& color);
		void setColor(std::optional<RGBA> color);
		[[nodiscard]] const std::optional<RGBA>& color() const { return mColor; }
		[[nodiscard]] RGBA shownColor() const { return mColor.value_or(mFallbackColor); }

		void openNewColorEditor();
		void closeNewColorEditor();
		[[nodiscard]] bool isNewColorEditorOpen() const;

		void setOnPicked(std::function<void(const RGBA&)> callback) { mOnPicked = std::move(callback); }
		void setOnPaletteChanged(std::function<void(const std::vector<RGBA>&)> callback) {
			mOnPaletteChanged = std::move(callback);
		}

		[[nodiscard]] ColorCell& swatch(size_t index) { return *mSwatches.at(index); }
		[[nodiscard]] ColorCell& addCell() { return mAddCell; }
		[[nodiscard]] Widget& grid() { return mGrid; }
		[[nodiscard]] ColorWheel& newColorWheel() { return mWheel; }
		[[nodiscard]] Button& confirmNewColorButton() { return mConfirmNewColor; }
		[[nodiscard]] Button& cancelNewColorButton() { return mCancelNewColor; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;

	private:
		void buildPalettePopup();
		void buildNewColorPopup();
		void rebuildCells();
		void pick(const RGBA& color);
		void addNewColor();
		void refreshSelection();

		Panel mPopup{ Panel::Surface::Raised };
		Widget mGrid;
		std::vector<std::unique_ptr<ColorCell>> mSwatches;
		ColorCell mAddCell{ ColorCell::Kind::Add };

		Panel mNewColorPopup{ Panel::Surface::Raised };
		ColorWheel mWheel;
		Widget mNewColorActions;
		Button mCancelNewColor{ "Cancel" };
		Button mConfirmNewColor{ "Add" };

		std::vector<RGBA> mPalette;
		std::optional<RGBA> mColor;
		RGBA mFallbackColor = { 0.5f, 0.5f, 0.5f, 1.0f };
		std::function<void(const RGBA&)> mOnPicked;
		std::function<void(const std::vector<RGBA>&)> mOnPaletteChanged;
	};
}
