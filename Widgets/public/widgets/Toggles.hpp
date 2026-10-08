#pragma once

#include "Control.hpp"

#include <string>
#include <vector>

namespace tp::ui {

	class ToggleControl : public Control {
	public:
		explicit ToggleControl(std::string text);

		void setChecked(bool checked);
		[[nodiscard]] bool isChecked() const { return mChecked; }

		void setOnChanged(std::function<void(bool)> onChanged) { mOnChanged = std::move(onChanged); }

		void setText(std::string text);
		[[nodiscard]] const std::string& text() const { return mText; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onActivated() override;
		void paint(Painter& painter) override;

		virtual void paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) = 0;
		[[nodiscard]] virtual halnf indicatorWidth() const;
		[[nodiscard]] virtual bool togglesOnActivate() const { return true; }

		[[nodiscard]] RectF indicatorBox() const;

	private:
		std::string mText;
		bool mChecked = false;
		AnimatedFloat mCheckedAmount;
		std::function<void(bool)> mOnChanged;
	};

	class Checkbox : public ToggleControl {
	public:
		explicit Checkbox(std::string text = {});

	protected:
		void paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) override;
	};

	class Switch : public ToggleControl {
	public:
		explicit Switch(std::string text = {});

	protected:
		void paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) override;
		[[nodiscard]] halnf indicatorWidth() const override;
	};

	class RadioGroup;

	class RadioButton : public ToggleControl {
	public:
		explicit RadioButton(std::string text = {});
		~RadioButton() override;

		[[nodiscard]] RadioGroup* group() const { return mGroup; }

	protected:
		void paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) override;
		[[nodiscard]] bool togglesOnActivate() const override { return false; }
		void onActivated() override;

	private:
		friend class RadioGroup;

		RadioGroup* mGroup = nullptr;
	};

	class RadioGroup {
	public:
		RadioGroup() = default;
		~RadioGroup();

		RadioGroup(const RadioGroup&) = delete;
		RadioGroup& operator=(const RadioGroup&) = delete;

		void add(RadioButton& button);
		void remove(RadioButton& button);

		void select(RadioButton& button);
		void selectIndex(size_t index);
		[[nodiscard]] long selectedIndex() const;
		[[nodiscard]] RadioButton* selected() const;

		void setOnChanged(std::function<void(size_t)> onChanged) { mOnChanged = std::move(onChanged); }

	private:
		std::vector<RadioButton*> mButtons;
		std::function<void(size_t)> mOnChanged;
	};
}
