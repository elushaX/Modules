#pragma once

#include "Library.hpp"
#include "Player.hpp"
#include "Widgets.hpp"

#include "imgui.h"

namespace tp {

	template <typename Events, typename Canvas>
	class TrackWidget : public Widget<Events, Canvas> {
	public:
		explicit TrackWidget(const Track* track = nullptr) :
			mTrack(track) {
			this->mArea.w = 70;
			col.mColor.setAnimTime(0);
			col.mColor.setNoTransition({ 0.15f, 0.15f, 0.15f, 0.f });
		};

		void proc(const Events& events, const RectF& areaParent, const RectF& area) override {
			mSelected = false;
			this->mArea = area;
			this->mVisible = area.isOverlap(areaParent);
			if (!this->mVisible) return;

			if (!mTrack) return;
			if (!areaParent.isOverlap(area)) return;
			if (area.isInside(events.getPointer())) {
				col.set({ 0.15f, 0.15f, 0.15f, 1.f });
				mSelected = events.isReleased(InputID::MOUSE1);
			} else {
				col.set({ 0.15f, 0.15f, 0.15f, 0.f });
			}
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			if (!mTrack) return;

			canvas.rect(this->mArea, col.get(), 4.f);

			const RectF imageArea = {
				this->mArea.x + margin, this->mArea.y + margin, this->mArea.w - margin * 2, this->mArea.w - margin * 2
			};

			canvas.rect(imageArea, { 0.25f, 0.25f, 0.25f, 1.f }, 4.f);

			const RectF textArea = { this->mArea.x + this->mArea.w + margin,
															 this->mArea.y + margin,
															 this->mArea.z - this->mArea.w - margin * 2,
															 this->mArea.w - margin * 2 };

			// canvas.rect(textArea, { 0.25f, 0.25f, 0.25f, 1.f }, 4.f);

			const RectF textAreaName = { textArea.x, textArea.y, textArea.z, textArea.w * 0.5f };
			const RectF textAreaAuthor = { textArea.x, textArea.y + textArea.w * 0.5f, textArea.z, textArea.w * 0.5f };

			canvas.text(mTrack->mName.read(), textAreaName, 15.f, Canvas::LC, 4.f, { 0.9f, 0.9f, 0.9f, 1.f });
			canvas.text(mTrack->mArtist.read(), textAreaAuthor, 12.f, Canvas::LC, 4.f, { 0.8f, 0.8f, 0.8f, 1.f });
		}

	public:
		halnf margin = 5.f;
		AnimColor col;
		const Track* mTrack;
		bool mSelected = false;
	};

	template <typename Events, typename Canvas>
	class TrackInfoWidget : public Widget<Events, Canvas> {
		struct SortType {
			String text;
			bool dec = false;
			bool inc = false;
		};

	public:
		explicit TrackInfoWidget(const Track* track = nullptr) :
			mTrack(track) {
			items.append({ "Date Added" });
			items.append({ "Date Last Played" });
		}

		void proc(const Events&, const RectF& areaParent, const RectF& area) override {
			this->mArea = area;
			this->mVisible = area.isOverlap(areaParent);
			if (!this->mVisible) return;
			if (!mTrack) return;
			// renderUI();
		}

		void draw(Canvas&) override {
			if (!this->mVisible) return;
			if (!mTrack) return;
			// canvas.rect(this->mArea, { 0.13f, 0.13f, 0.13f, 1.f }, 4.f);
			renderUI();
		}

		void renderUI() {
			ImGui::SetNextWindowPos({ this->mArea.x, this->mArea.y });
			ImGui::SetNextWindowSize({ this->mArea.z, this->mArea.w });

			ImGui::Begin(
				"InfoWindow",
				nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
					ImGuiWindowFlags_NoResize
			);

			if (mTrack) {

				ImGui::Text(" Load Status : %s", mLoadStatus ? "Loaded" : "Not Loaded");

				if (ImGui::Button("Play")) {
					if (mPlayer->getPlayingId() != mTrack->mId) {
						mLoadStatus = mPlayer->startStreamTrack(mTrack->mId);
					}
					mPlayer->continuePlayback();
				}

				if (mLoadStatus) {

					ImGui::SameLine();
					if (ImGui::Button("Stop")) {
						mPlayer->freezePlayback();
					}

					auto songProgress = mPlayer->getPlaybackProgress();
					auto vol = mPlayer->getVolume();
					ImGui::Text("Load Progress : %f", mPlayer->getLoadProgress());
					if (ImGui::SliderFloat("Progress", &songProgress, 0.f, 1.f)) {
						mPlayer->setPlaybackProgress(songProgress);
					}
					if (ImGui::SliderFloat("Volume", &vol, 0.f, 1.f)) {
						mPlayer->setVolume(vol);
					}
				}
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
				ImGui::Text("Album : %s", mTrack->mAlbum.read());
			} else {
				ImGui::Text("Not Selected");
			}

			ImGui::Separator();

			if (ImGui::Checkbox("Loved only", &filterLoved)) {
				isSongFilterChanged |= true;
			}

			if (ImGui::SliderInt("Existing only", &filterExisting, 0, 3)) {
				isSongFilterChanged |= true;
			}

			isSongFilterChanged |= songFilter.Draw("Song Filter");

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
		ImGuiTextFilter songFilter{};
		ImGuiTextFilter sortFilter{};
		Buffer<SortType> items;
		const Track* mTrack;
		bool filterLoved = false;
		Player* mPlayer = nullptr;
		bool mLoadStatus = false;
		bool isSongFilterChanged = true;
		int filterExisting = 0; // all existing no-existing
	};

	template <typename Events, typename Canvas>
	class LibraryWidget : public Widget<Events, Canvas> {
	public:
		LibraryWidget(Library* lib, Player* player) {
			mLibrary = (lib);
			mPlayer = (player);

			updateTracks();

			mCurrentTrackInfo.mPlayer = mPlayer;
		}

		void updateTracks() {
			mTracks.clear();
			for (auto track : mLibrary->mTraks) {
				mTracks.append(TrackWidget<Events, Canvas>(&track.data()));
			}
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = this->mArea.isOverlap(areaParent);
			if (!this->mVisible) return;

			filter();

			mSplitView.proc(events, this->mArea, this->mArea);
			mSongList.proc(events, this->mArea, mSplitView.getFirst());

			for (auto track : mSongList.mContents) {
				auto trackWidget = (TrackWidget<Events, Canvas>*) track.data();
				if (trackWidget->mSelected) {
					mCurrentTrackInfo.mTrack = trackWidget->mTrack;
				}
			}

			mCurrentTrackInfo.proc(events, this->mArea, mSplitView.getSecond());

			// mCurrentTrack.proc(events, this->mArea, mSplitView.getFirst());
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			canvas.rect(this->mArea, { 0.1f, 0.1f, 0.1f, 1.f });

			mSplitView.draw(canvas);
			mSongList.draw(canvas);
			mCurrentTrackInfo.draw(canvas);
		}

		void filter() {
			if (!mCurrentTrackInfo.isSongFilterChanged) return;

			mSongList.mContents.clear();

			for (auto track : mTracks) {
				if (!mCurrentTrackInfo.songFilter.PassFilter(track->mTrack->mName.read()) && !mCurrentTrackInfo.songFilter.PassFilter(track->mTrack->mArtist.read())) {
					continue;
				}

				if (mCurrentTrackInfo.filterLoved && !track->mTrack->mLoved) {
					continue;
				}

				switch (mCurrentTrackInfo.filterExisting) {
					case 1:
						if (!track->mTrack->mExists) continue;
						break;

					case 2:
						if (track->mTrack->mExists) continue;
						break;
				}

				mSongList.mContents.append(&track.data());
			}

			mCurrentTrackInfo.isSongFilterChanged = false;
		}

	private:
		Library* mLibrary = nullptr;
		Player* mPlayer = nullptr;

		Buffer<TrackWidget<Events, Canvas>> mTracks;

		SplitView<Events, Canvas> mSplitView;
		ScrollableWindow<Events, Canvas> mSongList;
		TrackInfoWidget<Events, Canvas> mCurrentTrackInfo;
		TrackWidget<Events, Canvas> mCurrentTrack;
	};
}