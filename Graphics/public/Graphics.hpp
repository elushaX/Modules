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

		void procBegin();
		void procEnd();

		void drawBegin();
		void drawEnd();
	};

	class Canvas {
		class Context;
		Context* mContext;

	public:
		explicit Canvas(Window* window);
		~Canvas();

		void procBegin() {}
		void procEnd() {}

		void drawBegin();
		void drawEnd();

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

		struct ColorWheel {
			HSV color = { 0.13, 0.5, 1 };
			halnf thickness = 20;
			halnf margin = 5;

			void fromPoint(const RectF& area, const Vec2F& point);
		};

		void setOrigin(const Vec2F& origin);

		void pushClamp(const RectF& rec);
		void popClamp();
		[[nodiscard]] const RectF& getClampedArea() const;

		void debugCross(RectF rec, const RGBA& col);
		void rect(RectF rec, const RGBA& col, halnf round = 0);
		void frame(RectF rec, const RGBA& col, halnf round = 0);
		void circle(Vec2F pos, halnf size, const RGBA& col);
		void text(const char*, const RectF&, halnf size, Align, halnf padding, const RGBA&);
		void colorWheel(const RectF& rec, const ColorWheel& colorWheel);

		ImageHandle createImageFromTextId(ualni id, Vec2F size);
		void updateTextureID(ImageHandle handle, ualni id);

		void deleteImageHandle(ImageHandle image);
		void drawImage(RectF rec, ImageHandle* image, halnf angle = 0, halnf alpha = 1.f, halnf rounding = 0.f);

	private:
		Buffer<RectF> mScissors;
		bool mIsClamping = false;
		Vec2F mOrigin = { 0, 0 };
	};

	class Graphics {
	public:
		explicit Graphics(Window* window) :
			mGui(window),
			mCanvas(window) {}

		void procBegin() {
			mCanvas.procBegin();
			mGui.procBegin();
		}

		void procEnd() {
			mCanvas.procEnd();
			mGui.procEnd();
		}

		void drawBegin() {
			mCanvas.drawBegin();
			mGui.drawBegin();
		}

		void drawEnd() {
			mCanvas.drawEnd();
			mGui.drawEnd();
		}

		Canvas* getCanvas() { return &mCanvas; }

	private:
		Canvas mCanvas;
		DebugGUI mGui;
	};
}