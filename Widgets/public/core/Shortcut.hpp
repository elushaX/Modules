#pragma once

#include "Event.hpp"

#include <string>
#include <vector>

namespace tp::ui {

	struct Shortcut {
		InputID key = InputID::SPACE;
		Modifiers modifiers;

		[[nodiscard]] bool matches(const Event& event) const;
		[[nodiscard]] std::string text() const;
		[[nodiscard]] bool operator==(const Shortcut& other) const;
	};

	Shortcut ctrl(InputID key);
	Shortcut ctrlShift(InputID key);
	Shortcut ctrlAlt(InputID key);
	Shortcut shift(InputID key);
	Shortcut alt(InputID key);
	Shortcut plain(InputID key);

	[[nodiscard]] std::string keyName(InputID key);

	template <typename CommandId>
	class ShortcutMap {
	public:
		struct Binding {
			Shortcut shortcut;
			CommandId command;
			bool repeats = false;
		};

		void bind(const Shortcut& shortcut, CommandId command, bool repeats = false) {
			unbind(shortcut);
			mBindings.push_back({ shortcut, command, repeats });
		}

		void unbind(const Shortcut& shortcut) {
			std::erase_if(mBindings, [&](const Binding& binding) { return binding.shortcut == shortcut; });
		}

		void clear() { mBindings.clear(); }

		[[nodiscard]] const Binding* find(const Event& event) const {
			if (event.type() != Event::Type::KeyDown) return nullptr;
			for (const Binding& binding : mBindings) {
				if (!binding.shortcut.matches(event)) continue;
				if (event.isRepeat() && !binding.repeats) return nullptr;
				return &binding;
			}
			return nullptr;
		}

		[[nodiscard]] std::vector<Shortcut> shortcutsFor(CommandId command) const {
			std::vector<Shortcut> shortcuts;
			for (const Binding& binding : mBindings) {
				if (binding.command == command) shortcuts.push_back(binding.shortcut);
			}
			return shortcuts;
		}

		[[nodiscard]] std::string textFor(CommandId command) const {
			const std::vector<Shortcut> shortcuts = shortcutsFor(command);
			return shortcuts.empty() ? std::string() : shortcuts.front().text();
		}

		[[nodiscard]] const std::vector<Binding>& bindings() const { return mBindings; }

	private:
		std::vector<Binding> mBindings;
	};
}
