
#pragma once

#include "Buffer2D.hpp"
#include "Color.hpp"
#include "Vec.hpp"

namespace tp {
	class RenderTexture {
	public:
		RenderTexture();
		~RenderTexture();

		uint4 getid();

		void update(const Buffer2D<RGBA>& buff);
		void draw(const uint4& out = 0);

	public:
		static void init();
		static void deinit();
		static void draw_texture(uint4 out, uint4 in);
		static uint4 get_tex(const char* TexId);
		static void drawCurcle(Vec2F pos, double radius, RGBA col);

	private:
		uint4 id;
	};
};