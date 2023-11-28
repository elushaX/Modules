#pragma once

// TODO : fix this ugly shit

#include "Buffer.hpp"
#include "Graphics.hpp"
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

		struct Events {
			Vec2F mPointer;
			
			const Vec2F& getPos() const;
			bool isPressed() const;
			bool isDown() const;
			halnf getScrollY() const;

		private:
			friend Window;
			Buffer<Event> mQueu;
			Context* mContext;
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

		Graphics::Canvas& getCanvas();
		const Events& getEvents();

	private:
		Context* mContext;
		Graphics mGraphics;
		Events mEvents;
	};
}