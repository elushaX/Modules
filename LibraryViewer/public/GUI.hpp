#pragma once

#include "Library.hpp"
#include "Player.hpp"
#include "Widgets.hpp"

#include "imgui.h"

namespace tp {

	class TrackWidget : public Widget {
	public:
		explicit TrackWidget(const Track* track = nullptr) :
			mTrack(track) {
			this->mArea.w = 70;
			col.mColor.setAnimTime(0);
			col.mColor.setNoTransition({ 0.15f, 0.15f, 0.15f, 0.f });
		};

		void eventProcess(const Events& events) override {
			mSelected = false;
			if (!mTrack) return;
			if (this->mArea.isInside(events.getPointer())) {
				col.set({ 0.15f, 0.15f, 0.15f, 1.f });
				mSelected = events.isReleased(InputID::MOUSE1);
			} else {
				col.set({ 0.15f, 0.15f, 0.15f, 0.f });
			}
		}

		void eventDraw(Canvas& canvas) override {
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

			canvas.text(mTrack->mName.c_str(), textAreaName, 15.f, Canvas::LC, 4.f, { 0.9f, 0.9f, 0.9f, 1.f });
			canvas.text(mTrack->mArtist.c_str(), textAreaAuthor, 12.f, Canvas::LC, 4.f, { 0.8f, 0.8f, 0.8f, 1.f });
		}

	public:
		halnf margin = 5.f;
		AnimColor col;
		const Track* mTrack;
		bool mSelected = false;
	};

	class TrackInfoWidget : public Widget {
		struct SortType {
			std::string text;
			bool dec = false;
			bool inc = false;
		};

	public:
		explicit TrackInfoWidget(const Track* track = nullptr) :
			mTrack(track) {
			items.append({ "Date Added" });
			items.append({ "Date Last Played" });
		}

		void eventDraw(Canvas&) override {
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
				ImGui::Text("Name : %s", mTrack->mName.c_str());
				ImGui::Text("Author : %s", mTrack->mArtist.c_str());
				ImGui::Text("Love : %s", mTrack->mLoved ? "true" : "false");
				ImGui::Text("Id : %lli", mTrack->mId);
				ImGui::Text("Total Time : %lli", mTrack->mTotalTime);
				ImGui::Text("Play Count : %lli", mTrack->mPlayCount);
				ImGui::Text("Skip Count : %lli", mTrack->mSkipCount);
				ImGui::Text("Date Added : %s", mTrack->mDateAdded.c_str());
				ImGui::Text("Album : %s", mTrack->mAlbum.c_str());
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
				if (!sortFilter.PassFilter(items[i].text.c_str())) continue;

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
				ImGui::Text("%s", items[i].text.c_str());
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

	class LibraryWidget : public WorkspaceWidget {
	public:
		LibraryWidget(Library* lib, Player* player) {
			mLibrary = (lib);
			mPlayer = (player);

			updateTracks();

			mCurrentTrackInfo.mPlayer = mPlayer;

			mDockSpace.setCenterWidget(&mSongList);
			mDockSpace.addSideWidget(&mCurrentTrackInfo, DockWidget::RIGHT);

			// this->mChildWidgets.pushBack(&mCurrentTrackInfo);
			// this->mChildWidgets.pushBack(&mCurrentTrack);
		}

		void updateTracks() {
			mTracks.clear();
			for (auto track : mLibrary->mTraks) {
				mTracks.append(TrackWidget(&track.data()));
			}
		}

		void eventProcess(const Events& events) override {
			filter();

			for (auto track : mSongList.getContent()) {
				auto trackWidget = (TrackWidget*) track.data();
				if (trackWidget->mSelected) {
					mCurrentTrackInfo.mTrack = trackWidget->mTrack;
				}
			}

			// mCurrentTrack.proc(events, this->mArea, mSplitView.getFirst());

			WorkspaceWidget::eventProcess(events);
		}

		// void eventDraw(Canvas& canvas) override {
			// canvas.rect(this->mArea, { 0.1f, 0.1f, 0.1f, 1.f });
		// }

		void filter() {
			if (!mCurrentTrackInfo.isSongFilterChanged) return;

			mSongList.clearContent();

			for (auto track : mTracks) {
				if (!mCurrentTrackInfo.songFilter.PassFilter(track->mTrack->mName.c_str()) &&
						!mCurrentTrackInfo.songFilter.PassFilter(track->mTrack->mArtist.c_str())) {
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

				mSongList.addWidget(&track.data());
			}

			mCurrentTrackInfo.isSongFilterChanged = false;
		}

	private:
		Library* mLibrary = nullptr;
		Player* mPlayer = nullptr;

		Buffer<TrackWidget> mTracks;

		ScrollableWindow mSongList;
		TrackInfoWidget mCurrentTrackInfo;
		TrackWidget mCurrentTrack;
	};
}