#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	class BreadcrumbPart : public Button {
	public:
		BreadcrumbPart(std::string text, bool current);

		[[nodiscard]] bool isCurrent() const { return mCurrent; }

	protected:
		[[nodiscard]] RGBA labelColor() const override;

	private:
		bool mCurrent;
	};

	class Breadcrumb : public Widget {
	public:
		Breadcrumb();

		void setParts(const std::vector<std::string>& parts);
		[[nodiscard]] size_t partCount() const { return mParts.size(); }
		[[nodiscard]] BreadcrumbPart& part(size_t index) { return *mParts[index]; }

		void setOnPartChosen(std::function<void(size_t)> callback) { mOnPartChosen = std::move(callback); }

	private:
		ScrollView mScroll{ Axis::Horizontal };
		Widget mRow;
		std::vector<std::unique_ptr<BreadcrumbPart>> mParts;
		std::vector<std::unique_ptr<Label>> mSeparators;
		std::function<void(size_t)> mOnPartChosen;
	};
}
