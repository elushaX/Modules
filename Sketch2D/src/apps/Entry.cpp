#include "Sketch2DApplication.hpp"
#include "ThreadPlacement.hpp"

int main() {
	tp::ThreadPlacement::reserveCallingThreadForInteraction();
	tp::sketch2d::Sketch2DOptions options;
	options.window = { MODULES_APP_ID, MODULES_APP_NAME };
	tp::sketch2d::Sketch2DApplication application(options);
	application.run();
	return 0;
}
