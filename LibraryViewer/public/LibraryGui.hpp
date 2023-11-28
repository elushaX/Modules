#pragma once

#include "LibraryViewer.hpp"
#include "Rect.hpp"
#include "Animations.hpp"

template <typename T>
concept DrawableConcept = true;

template <typename Events, typename Canvas> requires(DrawableConcept<Canvas>)
class ScrollBarWidget {
public:
	ScrollBarWidget() = default;

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& area) {
		if (mSizeFraction > 1.f) return;

		if (!areaParent.isOverlap(area)) {
			mIsScrolling = false;
			return;
		}

		if (events.getScrollY() != 0) {
			auto offset = events.getScrollY() < 0 ? 1.0f : -1.0f;
			if (scrollInertia * offset > 0) {
				scrollInertia += offset;
			} else {
				scrollInertia = -scrollInertia + offset;
			}
		}
		
		if (tp::abs(scrollInertia) > 0.1f) {
			auto offset = scrollInertia * mScrollFactor;
			mPositionFraction += offset;
			mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
			scrollInertia *= 0.f;
			return;	
		}

		if (events.isPressed() && area.isInside(events.getPos())) {
			mIsScrolling = true;		
		} else if (!events.isDown()) {
			mIsScrolling = false;
		}

		if (mIsScrolling) {
			tp::halnf pos = events.getPos().y;
			pos = (pos - area.y - mSizeFraction * area.w / 2.f) / area.w;
			mPositionFraction = tp::clamp(pos, 0.f, 1.f - mSizeFraction);
		}

		mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
	}

	void draw(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& area) { 
		if (mSizeFraction > 1.f) return;
		if (!areaParent.isOverlap(area)) return;
		
		auto minSize = 10.f;
		tp::RGBA col = tp::RGBA{ 0.25f, 0.25f, 0.25f, 1.f };

		if (mIsScrolling) {
			col = tp::RGBA{ 0.45f, 0.45f, 0.45f, 1.f };
			minSize = 20.f;
		}

		canvas.rect(area, { 0.15f, 0.15f, 0.15f, 1.f }, 4.f); 
		auto sliderSize = tp::clamp(area.w * mSizeFraction, minSize, area.w);
		auto diffSize = sliderSize - area.w * mSizeFraction;
		canvas.rect({ area.x, area.y + (area.w - diffSize) * mPositionFraction, area.z, sliderSize }, col, 4.f); 
	}

public:
	tp::halnf mScrollFactor = 0.f;
	tp::halnf scrollInertia = 0.f;
	bool mIsScrolling = false;
	tp::halnf mSizeFraction = 1.f;
	tp::halnf mPositionFraction = 0.f;
};


template <typename Events, typename Canvas> requires(DrawableConcept<Canvas>)
class TrackWidget {
public:
	TrackWidget(const Track* track) : mTrack(track) {
		col.mColor.setAnimTime(0);
		col.mColor.setNoTransition({ 0.15f, 0.15f, 0.15f, 0.f });
	};

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& area) {
		if (!areaParent.isOverlap(area)) return;
		if (area.isInside(events.getPos())) {
			col.set( { 0.15f, 0.15f, 0.15f, 1.f } );
			insideDebug = true;
		} else {
			col.set( { 0.15f, 0.15f, 0.15f, 0.f } );
			insideDebug = false;
		}
	}

	void draw(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& area) { 
		if (!areaParent.isOverlap(area)) return;

		canvas.rect(area, col.get(), 4.f); 

		const tp::RectF imageArea = { area.x + marging, area.y + marging, area.w - marging * 2, area.w - marging * 2 };
		canvas.rect(imageArea, { 0.25f, 0.25f, 0.25f, 1.f }, 4.f); 

		const tp::RectF textArea = { area.x + area.w + marging, area.y + marging, area.z - area.w - marging * 2, area.w - marging * 2 };
		// canvas.rect(textArea, { 0.25f, 0.25f, 0.25f, 1.f }, 4.f); 

		const tp::RectF textAreaName = { textArea.x, textArea.y, textArea.z, textArea.w * 0.5f };
		const tp::RectF textAreaAuthor = { textArea.x, textArea.y + textArea.w * 0.5f, textArea.z, textArea.w * 0.5f };
		
		canvas.text(mTrack->mName.read(), textAreaName, 15.f, Canvas::LC, 4.f, { 0.9f, 0.9f, 0.9f, 1.f} );
		canvas.text(mTrack->mArtist.read(), textAreaAuthor, 12.f, Canvas::LC, 4.f, { 0.8f, 0.8f, 0.8f, 1.f} );
	}

private:
	tp::halnf marging = 5.f;
	bool insideDebug = false;
	tp::AnimColor col;
	const Track* mTrack;
};

template <typename Events, typename Canvas> requires(DrawableConcept<Canvas>)
class LibraryWidget {
public:
	LibraryWidget(Library* lib) : mLibrary(lib) {
		for (auto track : mLibrary->mTraks) {
			mTraks.append(TrackWidget<Events, Canvas>(&track.data()));
		}
	}

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) {

		area = { aArea.x, aArea.y, aArea.z, aArea.w - controllPanelSize };
		areaCP = { aArea.x, aArea.y + aArea.w - controllPanelSize, aArea.z, controllPanelSize };

		mScroll.mSizeFraction = area.w / (mLibrary->mTraks.size() * trackSize);
		mScroll.mScrollFactor = 1.f / mLibrary->mTraks.size();

		if (area.isInside(events.getPos())) {
			
			mScroll.proc(events, area, { area.x + area.z - scrollSize - 3, area.y + 10, scrollSize - 3.f, area.w - 20 } );

			tp::halnf offset = mScroll.mPositionFraction * mLibrary->mTraks.size() * trackSize;
			auto idx = 0;

			for (auto track : mTraks) {
				track->proc(events, area, { area.x + 10, area.y + 10 + idx * trackSize - offset, area.z - 20 - scrollSize, trackSize - 5 } );
				idx++;
			}
		}

		debugPos = events.getPos();
	}

	void draw(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& aArea) { 
		canvas.rect(aArea, { 0.1f, 0.1f, 0.1f, 1.f }); 
		
		tp::halnf offset = mScroll.mPositionFraction * mLibrary->mTraks.size() * trackSize;
		auto idx = 0;

		for (auto track : mTraks) {
			track->draw(canvas, area, { area.x + 10, area.y + 10 + idx * trackSize - offset, area.z - 20 - scrollSize, trackSize - 5 } );
			idx++;
		}

		mScroll.draw(canvas, area, { area.x + area.z - scrollSize - 3, area.y + 10, scrollSize - 3.f, area.w - 20 } );

		drawCP(canvas, aArea, areaCP);
	}

	void drawCP(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& area) {
		canvas.rect( area, { 0.04f, 0.04f, 0.04f, 1.f });
		mTraks.first().draw(canvas, area, area);
	}

private:
	tp::halnf scrollSize = 15;
	tp::halnf controllPanelSize = 70;
	tp::halnf trackSize = 60;
	tp::Vec2F debugPos;

	tp::RectF area;
	tp::RectF areaCP;

	tp::Buffer<TrackWidget<Events, Canvas>> mTraks;
	ScrollBarWidget<Events, Canvas> mScroll;
	Library* mLibrary;
};