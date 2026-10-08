#include "Painter.hpp"

using namespace tp;
using namespace tp::ui;

std::vector<halnf> TextMetrics::prefixWidths(const std::string& text, const TextStyle& style) const {
	std::vector<halnf> widths(text.size() + 1, 0);
	for (size_t length = 1; length <= text.size(); length++)
		widths[length] = styledTextWidth(text.substr(0, length), style);
	return widths;
}
