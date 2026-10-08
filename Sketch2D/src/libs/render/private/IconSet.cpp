#include "IconSet.hpp"

namespace tp::sketch2d {

	namespace {
		const char* const kIconNames[] = {
			"brush",  "eraser",  "smudge",  "eyedropper", "line",  "move",   "wand",     "lasso",     "rectselect", "fill",
			"export", "history", "hand",    "tool",       "color", "size",   "zoom",     "undo",      "redo",       "select",
			"view",   "layers",  "library", "settings",   "menu",  "rotate", "gradient", "transform", "crop",       "edit",
		};
	}

	IconSet::IconSet(Canvas& canvas, const std::string& directory) :
		mCanvas(canvas) {
		for (const char* name : kIconNames) {
			const std::string path = directory + "/" + name + ".png";
			mIcons.put(hash(name), canvas.createImageFromFile(path.c_str()));
		}
	}

	IconSet::~IconSet() {
		for (auto entry : mIcons)
			mCanvas.deleteImageHandle(entry->val);
	}

	ui::ImageId IconSet::icon(const std::string& name) const {
		const auto slot = mIcons.presents(hash(name));
		if (!slot.isValid()) return {};
		return { mIcons.getSlotVal(slot).id };
	}

	ualni IconSet::hash(const std::string& name) {
		ualni value = 1469598103934665603ULL;
		for (char character : name) {
			value ^= (ualni) (unsigned char) character;
			value *= 1099511628211ULL;
		}
		return value;
	}
}
