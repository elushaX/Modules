#include "FloatingWidget.hpp"

namespace tp {
	class FloatingLayoutWidget : public Widget {
	public:
		FloatingLayoutWidget();

		void eventProcess(const Events& events) override;

		[[nodiscard]] bool handlesEvent() const;

	private:
		void updateActiveWindow(const tp::Events& events);

	private:
		bool mIsPassThrough = false;
	};
}