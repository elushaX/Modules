#pragma once

#include "GraphicsCommon.hpp"

// TODO : ugly

namespace tp {
	class Window;

	class Graphics {
	public:
		class GL {
			class Context;
			Context* mContext;

		private:
			friend Graphics;
			friend Window;

			GL();
			~GL();

			void init();
			void deinit();
			void proc();
			void draw();

		public:
			// TODO : API

		private:
			halnf mWidth = 100;
			halnf mHeight = 100;
		};

		class GUI {
			class Context;
			Context* mContext;

		private:
			friend Graphics;

			GUI();
			~GUI();

			void init(Window* window);
			void deinit();
			void proc();
			void draw();

		public:
			// TODO : API
		};

		class Canvas {
			class Context;
			Context* mContext;

		private:
			friend Graphics;
			friend Window;

			Canvas();
			~Canvas();

			void init();
			void deinit();
			void proc();
			void draw();

		public:
			enum Align : int2 {
				CC = 0x0000,
				CT = 0x0001,
				CB = 0x0002,
				LC = 0x0100,
				LT = 0x0101,
				LB = 0x0102,
				RC = 0x0200,
				RT = 0x0201,
				RB = 0x0202,
			};

			struct ImageHandle {
				ualni id = 0;
			};

			RectF getAvaliableArea();
			void pushClamp(const RectF& rec);
			void popClamp();
			void rect(const RectF& rec, const RGBA& col, halnf round = 0);
			void text(const char*, const RectF&, halnf size, Align, halnf marging, const RGBA&);

			ImageHandle createImageFromTextId(ualni id, Vec2F size); 
			void deleteImageHandle(ImageHandle image);
			void drawImage(const RectF& rec, ImageHandle* image, halnf angle, halnf alpha, halnf rounding);

		private:
			halnf mWidth = 600;
			halnf mHeight = 600;
			Buffer<RectF> mScissors;
			bool mIsClamping = false;
		};

	private:
		friend Window;

		Graphics() = default;
		void init(Window* window);
		void deinit();
		void draw();
		void proc();

	private:
		GUI mGui;
		GL mGl;
		Canvas mCanvas;
	};
}