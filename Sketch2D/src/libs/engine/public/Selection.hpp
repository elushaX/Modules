#pragma once

#include "Tile.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace tp::sketch2d {

	enum class SelectionMode { Replace, Add, Subtract };

	class Selection {
	public:
		struct MaskTile {
			uint1 coverage[kTileSize * kTileSize] = {};
		};

		[[nodiscard]] bool isActive() const { return mActive; }
		[[nodiscard]] bool isInverted() const { return mInverted; }
		[[nodiscard]] uint1 coverage(int4 x, int4 y) const;
		[[nodiscard]] halnf coverageAt(Vec2F world) const;
		[[nodiscard]] bool bounds(int4& left, int4& top, int4& right, int4& bottom) const;
		[[nodiscard]] size_t tileCount() const { return mTiles.size(); }

		void clear();
		void selectAll(int4 left, int4 top, int4 right, int4 bottom);
		void invert();
		void addRectangle(int4 left, int4 top, int4 right, int4 bottom, SelectionMode mode);
		void addEllipse(Vec2F center, Vec2F radius, SelectionMode mode);
		void addPolygon(const std::vector<Vec2F>& points, SelectionMode mode);
		void addMask(int4 left, int4 top, int4 width, const std::vector<uint1>& mask, SelectionMode mode);
		void spread(int4 pixels);
		void blur(int4 radius);

		[[nodiscard]] std::vector<std::pair<Vec2F, Vec2F>> outline() const;
		[[nodiscard]] bool operator==(const Selection& other) const;

	private:
		[[nodiscard]] const MaskTile* tile(int4 tx, int4 ty) const;
		MaskTile* writableTile(int4 tx, int4 ty);
		void combine(int4 x, int4 y, uint1 value, SelectionMode mode);
		void compact();

		static ualni key(int4 tx, int4 ty) { return ((ualni) (uint4) tx << 32) | (ualni) (uint4) ty; }

		std::unordered_map<ualni, std::shared_ptr<MaskTile>> mTiles;
		bool mActive = false;
		bool mInverted = false;
	};
}
