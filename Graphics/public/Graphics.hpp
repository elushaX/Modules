#pragma once

#include "Window.hpp"

#include "Color.hpp"
#include "Rect.hpp"

#include "Buffer.hpp"

namespace tp {

	class DebugGUI {
		class Context;
		Context* mContext;

	public:
		explicit DebugGUI(Window* window);
		~DebugGUI();

		void proc();
		void draw();
	};

	class Canvas {
		class Context;
		Context* mContext;

	public:
		explicit Canvas(Window* window);
		~Canvas();

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

		void pushClamp(const RectF& rec);
		void popClamp();
		void rect(const RectF& rec, const RGBA& col, halnf round = 0);
		void text(const char*, const RectF&, halnf size, Align, halnf padding, const RGBA&);

		ImageHandle createImageFromTextId(ualni id, Vec2F size);
		void deleteImageHandle(ImageHandle image);
		void drawImage(const RectF& rec, ImageHandle* image, halnf angle, halnf alpha, halnf rounding);

	private:
		Buffer<RectF> mScissors;
		bool mIsClamping = false;
	};

	class Graphics {
	public:
		explicit Graphics(Window* window) : mGui(window), mCanvas(window) {}

		void proc() {
			mCanvas.proc();
			mGui.proc();
		}

		void draw() {
			mCanvas.draw();
			mGui.draw();
		}

	private:
		Canvas mCanvas;
		DebugGUI mGui;
	};
}