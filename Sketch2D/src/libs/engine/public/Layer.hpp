#pragma once

#include "Map.hpp"
#include "Tile.hpp"

#include <memory>
#include <string>

namespace tp::sketch2d {

	enum class BlendMode : int4 {
		Normal = 0,
		Multiply = 1,
		Screen = 2,
		Overlay = 3,
		Lighten = 4,
		Darken = 5,
		ColorDodge = 6,
		ColorBurn = 7,
		SoftDodge = 8,
		SoftBurn = 9,
		Color = 10,
	};

	constexpr int4 kBlendModeCount = 11;

	[[nodiscard]] const char* blendModeName(BlendMode mode);
	[[nodiscard]] BlendMode nextBlendMode(BlendMode mode);
	[[nodiscard]] BlendMode previousBlendMode(BlendMode mode);

	class LayerLevels;

	class Layer {
	public:
		using TilePointer = std::shared_ptr<Tile>;

		struct TileSlot {
			TileCoord coord;
			TilePointer tile;
		};

		struct Properties {
			std::string name;
			bool visible = true;
			halnf opacity = 1.0f;
			BlendMode blend = BlendMode::Normal;
			bool clipped = false;
			bool locked = false;
			bool alphaLocked = false;
			bool selected = false;
		};

		struct State {
			ualni id = 0;
			Properties properties;
			Map<ualni, TileSlot> tiles;
		};

		Layer(ualni id, const std::string& name);
		~Layer();

		[[nodiscard]] LayerLevels& levels() const;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		[[nodiscard]] ualni id() const { return mId; }
		[[nodiscard]] ualni serial() const { return mSerial; }

		Tile* tileAt(const TileCoord& coord);
		[[nodiscard]] Tile* tileIfExists(const TileCoord& coord) const;
		[[nodiscard]] std::shared_ptr<Tile> sharedTile(const TileCoord& coord) const;
		std::shared_ptr<Tile> replaceTile(const TileCoord& coord, std::shared_ptr<Tile> tile);
		[[nodiscard]] ualni tileCount() const { return mTiles.size(); }

		template <typename Fn>
		void forEachTile(Fn&& fn) const {
			for (auto it : mTiles)
				fn(it->val.coord, it->val.tile.get());
		}

		void clear();
		void removeTile(const TileCoord& coord);

		void noteDisplayChanged(const TileCoord& coord) { mChanged.put(coord.key(), coord); }

		void noteChanged(const TileCoord& coord) {
			mChanged.put(coord.key(), coord);
			mLevelChanged.put(coord.key(), coord);
		}
		template <typename Fn>
		void forEachLevelChanged(Fn&& fn) const {
			for (auto it : mLevelChanged)
				fn(it->val);
		}
		void clearLevelChanges() const { mLevelChanged.removeAll(); }
		template <typename Fn>
		void forEachChanged(Fn&& fn) const {
			for (auto it : mChanged)
				fn(it->val);
		}
		[[nodiscard]] ualni changedCount() const { return mChanged.size(); }
		void clearChanges() const { mChanged.removeAll(); }

		[[nodiscard]] State state() const;
		void restore(const State& state);

		[[nodiscard]] const Properties& properties() const { return mProperties; }
		[[nodiscard]] const std::string& name() const { return mProperties.name; }
		void setName(const std::string& name) { mProperties.name = name; }
		[[nodiscard]] bool visible() const { return mProperties.visible; }
		void setVisible(bool visible) { mProperties.visible = visible; }
		[[nodiscard]] halnf opacity() const { return mProperties.opacity; }
		void setOpacity(halnf opacity) { mProperties.opacity = opacity; }
		[[nodiscard]] BlendMode blend() const { return mProperties.blend; }
		void setBlend(BlendMode blend) { mProperties.blend = blend; }
		[[nodiscard]] bool clipped() const { return mProperties.clipped; }
		void setClipped(bool clipped) { mProperties.clipped = clipped; }
		[[nodiscard]] bool locked() const { return mProperties.locked; }
		void setLocked(bool locked) { mProperties.locked = locked; }
		[[nodiscard]] bool alphaLocked() const { return mProperties.alphaLocked; }
		void setAlphaLocked(bool alphaLocked) { mProperties.alphaLocked = alphaLocked; }
		[[nodiscard]] bool selected() const { return mProperties.selected; }
		void setSelected(bool selected) { mProperties.selected = selected; }

	private:
		ualni mId;
		ualni mSerial;
		Properties mProperties;
		Map<ualni, TileSlot> mTiles;
		mutable Map<ualni, TileCoord> mChanged;
		mutable Map<ualni, TileCoord> mLevelChanged;
		mutable std::unique_ptr<LayerLevels> mLevels;
	};
}
