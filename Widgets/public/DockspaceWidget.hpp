#include "FloatingWidget.hpp"

namespace tp {
	class DockSpaceWidget : public Widget {
	public:
		DockSpaceWidget() = default;

		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;
	};
}