
// #include "Fram.h"

class Widget {
	void destructor(StrokesWidget* self) {
		self->mRenderer.~Renderer();
		self->mImage.free(self->mDrawer);
		self->mImage.~ImageHandle();
	}

	void procInputs(StrokesWidget* self, nd::GUIInputs* inputs) {
		auto proj = self->getTargetProject();
		if (!proj) {
			return;
		}

		auto rec = self->getRect();
		proj->mCamera.set_ratio(rec.w / rec.z);

		auto col_obj_bg = self->getMember<obj::ColorObject>("bg_color");
		if (col_obj_bg) {
			proj->mBackgroundColor = col_obj_bg->mCol;
		}

		if (!rec.inside(inputs->mCrsPrev)) {
			return;
		}

		auto pressure = inputs->mPressure;
		if (!inputs->Anticipating()) {
			pressure = 0.f;
		}

		auto idx = proj->mBrushes.presents(proj->mActiveBrush);
		if (idx) {
			auto brush = proj->mBrushes.getSlotVal(idx);
			auto crs = (inputs->mCrs - rec.pos);
			crs.x /= rec.z;
			crs.y /= rec.w;
			crs = (crs - 0.5) * 2;

			if (brush->mType == "pencil") {
				auto col_obj = self->getMember<obj::ColorObject>("draw_color");
				if (col_obj) {
					((strokes::PencilBrush*) brush)->mCol = col_obj->mCol;
				}
			}

			brush->sample(proj, crs, pressure);
		}
	}

	void presentOutput(StrokesWidget* self, nd::GUIdrawer* drawer) {
		auto proj = self->getTargetProject();
		if (!proj) {
			return;
		}

		if (!self->mImage.mId) {
			self->mDrawer = drawer;
			self->mImage.createFromBuff(self->mRenderer.getBuff(), drawer);
		}

		auto rec = self->getRect();
		self->mRenderer.setViewport({ 0, 0, rec.z, rec.w });
		self->mRenderer.setClearCol(proj->mBackgroundColor);
		self->mRenderer.renderBegin();

		for (auto lay : proj->mLayers) {
			if (lay.data()->enabled) {
				for (auto str : lay.data()->strokes) {
					self->mRenderer.drawStroke(str.data(), &proj->mCamera);
				}
			}
		}

		auto idx = proj->mBrushes.presents(proj->mActiveBrush);
		if (idx) {
			auto brush = proj->mBrushes.getSlotVal(idx);
			brush->draw(&self->mRenderer, &proj->mCamera);
		}

		self->mRenderer.renderEnd();

		drawer->drawImage(rec, &self->mImage, 0, 1, 12);
	}
};