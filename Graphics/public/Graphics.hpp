#include "Strings.hpp"

namespace tp {

	class ShowoffGlContext;
	class ShowoffGUIContext;
	class ShowoffCanvasContext;

	class Window;
	class WindowContext;

	class Showoff {
	public:
		class GL {
			ShowoffGlContext* mContext;
		public:
			GL();
			~GL();

			void init();
			void deinit();
			static void beginDraw();
			static void endDraw();
		};

		class GUI {
			ShowoffGUIContext* mContext;
		public:
			GUI();
			~GUI();

			void init(Window* window);
			void deinit();
			void beginDraw();
			void endDraw();
		};

		class Canvas {
			ShowoffCanvasContext* mContext;
		public:
			Canvas();
			~Canvas();

			void init();
			void deinit();
			void beginDraw();
			void endDraw();
		};

	public:
		Showoff() = default;
		void init(Window* window);
		void deinit();
		void draw();

	private:
		GUI gui;
		GL gl;
		Canvas canvas;
	};

	class Window {
		Window(int width, int height, const char* title);
		~Window();

	public:
		static Window* createWindow(int width, int height, const char* title);
		static void destroyWindow(Window* window);

	public:
		void renderLoop();
		WindowContext* getContext();
	private:
		WindowContext* mContext;
		Showoff showoff;
	};
}