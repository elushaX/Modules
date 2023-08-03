#pragma once

#include "Graphics.hpp"
#include "Buffer.hpp"
#include "Keycodes.hpp"

namespace tp {

	class Window {
		class Context;

	public:
		struct Event {
			enum Type {
				KEY,
				MOUSE,

			};
		};

	private:
		Window(int width, int height, const char* title);
		~Window();

	public:
		static Window* createWindow(int width, int height, const char* title);
		static void destroyWindow(Window* window);

	public:
		void draw();
		void processEvents();
		[[nodiscard]] bool shouldClose() const;

		auto getContext() -> Context*;

	private:
		Context* mContext;
		Graphics mGraphics;

		Buffer<Event> mEvents;
	};
}