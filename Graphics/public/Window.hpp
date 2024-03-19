#pragma once

#include "Buffer.hpp"
#include "EventHandler.hpp"
#include "Strings.hpp"

namespace tp {

	extern ModuleManifest gModuleGraphics;

	class Window {
		class Context;

	private:
		Window(Vec2F size, const String& title);
		~Window();

	public:
		static Window* createWindow(Vec2F size = { 1000.f, 900.f }, const String& title = "Window");
		static void destroyWindow(Window* window);

	public:
		void draw();
		void processEvents();

		void setEventHandler(EventHandler* eventHandler);
		[[nodiscard]] EventHandler* getEventHandler();

		Context* getContext();
		[[nodiscard]] bool shouldClose() const;
		[[nodiscard]] const Vec2F& getSize() const;

	private:
		Vec2F mSize;
		Context* mContext;
		EventHandler* mEventHandler = nullptr;
	};
}