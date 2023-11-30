#pragma once

#include "LibraryViewer.hpp"
#include "WavPlayer.hpp"
#include "Rect.hpp"
#include "Animations.hpp"
#include "imgui.h"

template <typename T>
concept DrawableConcept = true;

template <typename Events, typename Canvas> requires(DrawableConcept<Canvas>)
class ResizerWidget {
public:
	ResizerWidget() {}

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& area) {
		if (!areaParent.isOverlap(area)) {
			resizing = false;
			return;
		}

		hover = area.isInside(events.getPos());

		if (events.isPressed() && hover) {
			resizing = true;		
		} else if (!events.isDown()) {
			resizing = false;
		}

		value = area.x;

		if (resizing) {
			tp::halnf pos = events.getPos().x;
			auto diff = (pos - value - area.z / 2.f);
			value += diff;
		}

		value = tp::clamp(value, min, max);

		if (min > max) {
			value = min;
		}
	}

	void draw(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& area) { 
		if (!areaParent.isOverlap(area)) return;
		
		if (hover || resizing) {
			canvas.rect(area, { 0.23f, 0.23f, 0.23f, 1.f}, 0.f);
		} else {
			canvas.rect(area, { 0.04f, 0.04f, 0.04f, 1.f }, 0.f);
		}
	}

public:
	bool hover = false;
	bool resizing = false;
	tp::halnf max = 0;
	tp::halnf min = 0;
	tp::halnf value = 0;
};

template <typename Events, typename Canvas> requires(DrawableConcept<Canvas>)
class TrackInfoWidget {
	struct SortType {
		tp::String text;
		bool dec = false;
		bool inc = false;
	};

public:
	TrackInfoWidget(const Track* track = nullptr) : mTrack(track) {
		items.append( { "Date Added" } );
		items.append( { "Date Last Played" } );
	}

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& area) {
		if (!mTrack) return;
		if (!areaParent.isOverlap(area)) return;
	}

	void draw(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& area) { 
		if (!areaParent.isOverlap(area)) return;
		
		ImGui::SetNextWindowPos( { area.x, area.y } );
		ImGui::SetNextWindowSize( { area.z, area.w } );
		RenderUI();

		canvas.rect(area, { 0.13f, 0.13f, 0.13f, 1.f}, 4.f);

		if (!mTrack) return;
	}

	void RenderUI() {
		ImGui::Begin("InfoWindow", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize);

		if (mTrack && ImGui::Button("load")) {
			mLoadStatus = mPlayer->startStreamTrack(mTrack->mId);
		}

		ImGui::SameLine(); ImGui::Text(" Load Status : %s", mLoadStatus ? "Loaded" : "Not Loaded");

		if (mTrack && mLoadStatus) {
			auto songProgress = mPlayer->getProgress();
			ImGui::SliderFloat("Progress", &songProgress, 0.f, 1.f);
			mPlayer->setProgress(songProgress);
		}

		if (ImGui::Button("Play")) {
			mPlayer->playSong();
		}

		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			mPlayer->stopSong();
		}

		ImGui::Text("Song Info:");
		if (mTrack) {
			ImGui::Text("Name : %s", mTrack->mName.read());
			ImGui::Text("Author : %s", mTrack->mArtist.read());
			ImGui::Text("Love : %s", mTrack->mLoved ? "true" : "false");
			ImGui::Text("Id : %lli", mTrack->mId);
			ImGui::Text("Total Time : %lli", mTrack->mTotalTime);
			ImGui::Text("Play Count : %lli", mTrack->mPlayCount);
			ImGui::Text("Skip Count : %lli", mTrack->mSkipCount);
			ImGui::Text("Date Added : %s", mTrack->mDateAdded.read());
			ImGui::Text("Albom : %s", mTrack->mAlbum.read());
		} else {
			ImGui::Text("Not Selected");
		}

		ImGui::Separator();

		ImGui::Checkbox("Loved only", &filterLoved);
		songFilter.Draw("Song Filter");
		
		sortFilter.Draw("Sorting Type");

		for (int i = 0; i < items.size(); ++i) {
			if (!sortFilter.PassFilter(items[i].text.read())) continue;

			ImGui::PushID(i);
			
			if (ImGui::Button("Inc")) {
				items[i].inc = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Dec")) {
				items[i].dec = true;
			}

			ImGui::AlignTextToFramePadding();
			ImGui::SameLine();
			ImGui::Text("%s", items[i].text.read());
			ImGui::PopID();
		}

		ImGui::End();
	}

public:
	ImGuiTextFilter songFilter;
	ImGuiTextFilter sortFilter;
	tp::Buffer<SortType> items;
	const Track* mTrack;
	bool filterLoved = false;
	TrackPlayer* mPlayer = nullptr;
	bool mLoadStatus = false;
};


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

		if (events.getScrollY() != 0 && areaParent.isInside(events.getPos())) {
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
	TrackWidget(const Track* track = nullptr) : mTrack(track) {
		col.mColor.setAnimTime(0);
		col.mColor.setNoTransition({ 0.15f, 0.15f, 0.15f, 0.f });
	};

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& area) {
		if (!mTrack) return;
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
		if (!mTrack) return;
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

public:
	tp::halnf marging = 5.f;
	bool insideDebug = false;
	tp::AnimColor col;
	const Track* mTrack;
};

template <typename Events, typename Canvas> requires(DrawableConcept<Canvas>)
class LibraryWidget {
public:
	LibraryWidget(Library* lib, TrackPlayer* player) : mLibrary(lib), mPlayer(player) {
		for (auto track : mLibrary->mTraks) {
			mTraks.append(TrackWidget<Events, Canvas>(&track.data()));
		}
		mResizeWidget.value = 1000;
		mCurrentTrackInfo.mPlayer = mPlayer;
	}

	void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) {
		mNeedRedraw = events.isEvent();
		if (!mNeedRedraw) return;

		mResizeWidget.max = aArea.z - trackInfoSize;
		mResizeWidget.min = 100;

		areaCP = { aArea.x, aArea.y + aArea.w - controllPanelSize, aArea.z, controllPanelSize };
		area = { aArea.x, aArea.y, mResizeWidget.value, aArea.w - controllPanelSize };
		areaInfo = { mResizeWidget.value + resizeSize, aArea.y, aArea.z - mResizeWidget.value - resizeSize, aArea.w - controllPanelSize } ;
		areaResize = { mResizeWidget.value, aArea.y, resizeSize, areaInfo.w };

		mScroll.mSizeFraction = area.w / (mLibrary->mTraks.size() * trackSize);
		mScroll.mScrollFactor = 1.f / mLibrary->mTraks.size();

		mScroll.proc(events, area, { area.x + area.z - scrollSize - 3, area.y + 10, scrollSize - 3.f, area.w - 20 } );
		mResizeWidget.proc(events, area, areaResize );

		if (area.isInside(events.getPos())) {

			tp::halnf offset = mScroll.mPositionFraction * mLibrary->mTraks.size() * trackSize;
			auto idx = 0;

			for (auto track : mTraks) {
				auto trackArea = tp::RectF{ area.x + 10, area.y + 10 + idx * trackSize - offset, area.z - 20 - scrollSize, trackSize - 5 };

				track->proc(events, area, trackArea);

				if (trackArea.isInside(events.getPos()) && events.isPressed()) {
					mCurrentTrack.mTrack = track->mTrack;
					mCurrentTrackInfo.mTrack = track->mTrack;
				}

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
		
		// canvas.rect( areaInfo, { 0.04f, 0.04f, 0.04f, 1.f });
		mCurrentTrackInfo.draw(canvas, aArea, areaInfo );

		mResizeWidget.draw(canvas, aArea, areaResize );

		drawCP(canvas, aArea, areaCP);
	}

	void drawCP(Canvas& canvas, const tp::RectF& areaParent, const tp::RectF& area) {
		canvas.rect( area, { 0.04f, 0.04f, 0.04f, 1.f });
		mCurrentTrack.draw(canvas, area, area);
	}

private:
	tp::halnf scrollSize = 20;
	tp::halnf trackInfoSize = 200;
	tp::halnf controllPanelSize = 70;
	tp::halnf trackSize = 60;
	tp::Vec2F debugPos;
	tp::halnf resizeSize = 10;

	tp::RectF area;
	tp::RectF areaInfo;
	tp::RectF areaCP;
	tp::RectF areaResize;

	tp::Buffer<TrackWidget<Events, Canvas>> mTraks;
	TrackWidget<Events, Canvas> mCurrentTrack;
	TrackInfoWidget<Events, Canvas> mCurrentTrackInfo;

	ScrollBarWidget<Events, Canvas> mScroll;
	ResizerWidget<Events, Canvas> mResizeWidget;

	Library* mLibrary;
	TrackPlayer* mPlayer;

	bool mNeedRedraw = false;
};