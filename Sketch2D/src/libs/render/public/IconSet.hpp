#pragma once

#include "Graphics.hpp"
#include "Sketch2DUI.hpp"
#include "Map.hpp"

#include <string>

namespace tp::sketch2d {

	class IconSet : public IconProvider {
	public:
		IconSet(Canvas& canvas, const std::string& directory);
		~IconSet() override;

		IconSet(const IconSet&) = delete;
		IconSet& operator=(const IconSet&) = delete;

		[[nodiscard]] ui::ImageId icon(const std::string& name) const override;

	private:
		static ualni hash(const std::string& name);

		Canvas& mCanvas;
		Map<ualni, Canvas::ImageHandle> mIcons;
	};
}
