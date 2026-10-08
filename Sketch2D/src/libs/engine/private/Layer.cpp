#include "Layer.hpp"
#include "LayerLevels.hpp"

#include <atomic>

namespace tp::sketch2d {

	namespace {
		ualni nextSerial() {
			static std::atomic<ualni> serial{ 1 };
			return serial.fetch_add(1);
		}
	}

	const char* blendModeName(BlendMode mode) {
		switch (mode) {
			case BlendMode::Normal: return "Normal";
			case BlendMode::Multiply: return "Multiply";
			case BlendMode::Screen: return "Screen";
			case BlendMode::Overlay: return "Overlay";
			case BlendMode::Lighten: return "Lighten";
			case BlendMode::Darken: return "Darken";
			case BlendMode::ColorDodge: return "Color dodge";
			case BlendMode::ColorBurn: return "Color burn";
			case BlendMode::SoftDodge: return "Soft dodge";
			case BlendMode::SoftBurn: return "Soft burn";
			case BlendMode::Color: return "Color";
		}
		return "Normal";
	}

	BlendMode nextBlendMode(BlendMode mode) { return (BlendMode) (((int4) mode + 1) % kBlendModeCount); }

	BlendMode previousBlendMode(BlendMode mode) {
		return (BlendMode) (((int4) mode + kBlendModeCount - 1) % kBlendModeCount);
	}

	Layer::Layer(ualni id, const std::string& name) :
		mId(id),
		mSerial(nextSerial()) {
		mProperties.name = name;
	}

	Layer::~Layer() = default;

	LayerLevels& Layer::levels() const {
		if (!mLevels) mLevels = std::make_unique<LayerLevels>();
		return *mLevels;
	}

	void Layer::clear() {
		for (auto it : mTiles)
			noteChanged(it->val.coord);
		mTiles.removeAll();
	}

	void Layer::removeTile(const TileCoord& coord) {
		if (!mTiles.presents(coord.key()).isValid()) return;
		mTiles.remove(coord.key());
		noteChanged(coord);
	}

	Tile* Layer::tileIfExists(const TileCoord& coord) const {
		auto index = mTiles.presents(coord.key());
		if (!index.isValid()) return nullptr;
		return mTiles.getSlotVal(index).tile.get();
	}

	std::shared_ptr<Tile> Layer::sharedTile(const TileCoord& coord) const {
		auto index = mTiles.presents(coord.key());
		return index.isValid() ? mTiles.getSlotVal(index).tile : nullptr;
	}

	Tile* Layer::tileAt(const TileCoord& coord) {
		noteChanged(coord);
		auto index = mTiles.presents(coord.key());
		if (!index.isValid()) {
			mTiles.put(coord.key(), { coord, std::make_shared<Tile>() });
			return mTiles.get(coord.key()).tile.get();
		}

		TileSlot& slot = mTiles.get(coord.key());
		if (slot.tile.use_count() > 1) slot.tile = slot.tile->takeForWriting();
		return slot.tile.get();
	}

	Layer::State Layer::state() const { return { mId, mProperties, mTiles }; }

	std::shared_ptr<Tile> Layer::replaceTile(const TileCoord& coord, std::shared_ptr<Tile> tile) {
		noteChanged(coord);
		std::shared_ptr<Tile> previous;
		auto index = mTiles.presents(coord.key());
		if (index.isValid()) {
			TileSlot& slot = mTiles.getSlotVal(index);
			previous = slot.tile;
			if (previous.use_count() <= 2) {
				tile->setGpuId(previous->gpuId());
				previous->setGpuId(0);
			}
			slot.tile = std::move(tile);
		} else {
			mTiles.put(coord.key(), { coord, std::move(tile) });
		}
		return previous;
	}

	void Layer::restore(const State& state) {
		for (auto it : mTiles) {
			auto found = state.tiles.presents(it->key);
			if (!found.isValid() || state.tiles.getSlotVal(found).tile != it->val.tile) noteChanged(it->val.coord);
		}
		for (auto it : state.tiles) {
			if (!mTiles.presents(it->key).isValid()) noteChanged(it->val.coord);
		}
		mProperties = state.properties;
		mTiles = state.tiles;
	}
}
