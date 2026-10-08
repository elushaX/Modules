#pragma once

#include "Window.hpp"

#include "Color.hpp"
#include "Rect.hpp"

#include "Buffer.hpp"

struct NVGcontext;

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
		void drawWindowChrome();

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

		enum class FontFace { Regular, Bold, Italic, BoldItalic, Monospace };

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
		[[nodiscard]] bool isOutsideClamp(const RectF& rec) const;

		void debugCross(RectF rec, const RGBA& col);
		void rect(RectF rec, const RGBA& col, halnf round = 0);
		void gradientRect(RectF rec, const RGBA& from, const RGBA& to, bool horizontal, halnf round = 0);
		void frame(RectF rec, const RGBA& col, halnf round = 0);
		void outline(RectF rec, const RGBA& col, halnf round, halnf width);
		void circle(Vec2F pos, halnf size, const RGBA& col);
		void circleOutline(Vec2F pos, halnf size, const RGBA& col, halnf width);
		void polyline(const Vec2F* points, int count, const RGBA& col, halnf width);
		void polygon(const Vec2F* points, int count, const RGBA& col);
		void
		text(const char*, const RectF&, halnf size, Align, halnf padding, const RGBA&, FontFace face = FontFace::Regular);
		void rotatedText(const char* text, Vec2F center, halnf size, halnf angle, const RGBA& col);
		[[nodiscard]] halnf textWidth(const char*, halnf size, FontFace face = FontFace::Regular);
		void prefixWidths(const char* text, size_t length, halnf size, FontFace face, halnf* widths);
		void colorWheel(const RectF& rec, const ColorWheel& colorWheel);
		void line(Vec2F start, Vec2F end, const RGBA& col, halnf thickness);

		ImageHandle createImageFromTextId(ualni id, Vec2F size);
		void updateTextureID(ImageHandle handle, ualni id);

		ImageHandle createImageFromFile(const char* path);
		[[nodiscard]] Vec2F imageSize(ImageHandle image);
		ImageHandle createImageFromMemoryRGBA(int width, int height, const unsigned char* rgba);
		void updateImageRGBA(ImageHandle image, const unsigned char* rgba);

		void deleteImageHandle(ImageHandle image);
		void drawImage(RectF rec, ImageHandle* image, halnf angle = 0, halnf alpha = 1.f, halnf rounding = 0.f);

	private:
		[[nodiscard]] int fontIdFor(FontFace face) const;
		void fillWindowBase(const WindowFrame& frame);
		static void cutRoundedCorners(NVGcontext* vg, const WindowFrame& frame);
		static void paintOutline(NVGcontext* vg, const WindowFrame& frame);
		static void paintShadow(NVGcontext* vg, const WindowFrame& frame, bool focused);

		Buffer<RectF> mScissors;
		bool mIsClamping = false;
		Vec2F mOrigin = { 0, 0 };
	};

	class Graphics {
	public:
		explicit Graphics(Window* window) :
			mWindow(window),
			mCanvas(window),
			mGui(window) {}

		void procBegin() {
			mCanvas.procBegin();
			mGui.procBegin();
		}

		void procEnd() {
			mCanvas.procEnd();
			mGui.procEnd();
		}

		void drawBegin() {
			mFrameActive = mWindow->frameBegin();
			if (!mFrameActive) return;
			mCanvas.drawBegin();
			mGui.drawBegin();
		}

		void drawEnd() {
			if (!mFrameActive) return;
			mCanvas.drawEnd();
			mGui.drawEnd();
			mCanvas.drawWindowChrome();
			mWindow->frameEnd();
			mFrameActive = false;
		}

		Canvas* getCanvas() { return &mCanvas; }

	private:
		Window* mWindow;
		Canvas mCanvas;
		DebugGUI mGui;
		bool mFrameActive = false;
	};
}