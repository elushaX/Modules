#pragma once

#include "NodeGraph.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace tp::taskboard {

	[[nodiscard]] inline std::string lowercaseText(std::string text) {
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
		return text;
	}

	[[nodiscard]] inline std::optional<ui::NodeId> taskIdQuery(const std::string& query) {
		const std::string digits = query.starts_with('#') ? query.substr(1) : query;
		if (digits.empty() || !std::all_of(digits.begin(), digits.end(), [](unsigned char c) { return std::isdigit(c); }))
			return std::nullopt;
		return ui::NodeId(std::stoull(digits));
	}
}
