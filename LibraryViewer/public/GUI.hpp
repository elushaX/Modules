#pragma once

#include "Library.hpp"
#include "Player.hpp"
#include "FloatingWidget.hpp"
#include "DockWidget.hpp"

#include "imgui.h"

namespace tp {

	class TrackWidget : public Widget {
	public:
		enum State {
			IDLE,
			HOVER,
			SELECTED,
		};

		explicit TrackWidget(const Track* track = nullptr) :
			mTrack(track) {

			getLayout()->setMinSize({ 70, 70 });
		};

		void process(const EventHandler& events) override {
			mState = IDLE;

			if (!mTrack) {
				mState = IDLE;
				return;
			}

			switch (mState) {
				case HOVER:
				case IDLE:
					mState = getArea().relative().isInside(events.getPointer()) ? HOVER : IDLE;
					if (events.isPressed(InputID::MOUSE1)) {
						mState = SELECTED;
					}
					break;

				case SELECTED:
					break;
			}
		}

		void draw(Canvas& canvas) override {
			if (!mTrack) return;
			auto area = getArea().relative();

			switch (mState) {
				case IDLE: canvas.rect(area, colorIdle, rounding); break;
				case HOVER: canvas.rect(area, colorHover, rounding); break;
				case SELECTED: canvas.rect(area, colorSelected, rounding); break;
			}


			const RectF imageArea = { area.x + margin, area.y + margin, area.w - margin * 2, area.w - margin * 2 };
			canvas.rect(imageArea, colorImage, rounding);

			const RectF textArea = {
				area.x + area.w + margin, area.y + margin, area.z - area.w - margin * 2, area.w - margin * 2
			};

			// canvas.rect(textArea, { 0.25f, 0.25f, 0.25f, 1.f }, 4.f);

			const RectF textAreaName = { textArea.x, textArea.y, textArea.z, textArea.w * 0.5f };
			const RectF textAreaAuthor = { textArea.x, textArea.y + textArea.w * 0.5f, textArea.z, textArea.w * 0.5f };

			canvas.text(mTrack->mName.c_str(), textAreaName, 15.f, Canvas::LC, rounding, colorNameTrack);
			canvas.text(mTrack->mArtist.c_str(), textAreaAuthor, 12.f, Canvas::LC, rounding, colorNameArtist);
		}

		[[nodiscard]] bool processesEvents() const override { return true; }

	public:
		const Track* mTrack;
		State mState = IDLE;

	private:
		halnf margin = 5.f;
		halnf rounding = 5.f;

		RGBA colorIdle = 0;
		RGBA colorNameTrack = { 0.9f, 0.9f, 0.9f, 1.f };
		RGBA colorNameArtist = { 0.8f, 0.8f, 0.8f, 1.f };
		RGBA colorImage = { 0.5, 0.5, 0.5, 1.0 };
		RGBA colorHover = { 0.13, 0.13, 0.13, 0.9 };
		RGBA colorSelected = { 0.43, 0.43, 0.43, 0.9 };
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

		void draw(Canvas&) override {
			if (!mTrack) return;
			// canvas.rect(this->mArea, { 0.13f, 0.13f, 0.13f, 1.f }, 4.f);
			renderUI();
		}

		void renderUI() {
			// auto area = getArea();

			// ImGui::SetNextWindowPos({ area.x, this->mArea.y });
			// ImGui::SetNextWindowSize({ area.z, this->mArea.w });

			ImGui::Begin(
				"InfoWindow",
				nullptr //,
				//ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
				//	ImGuiWindowFlags_NoResize
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

	class LibraryWidget : public DockWidget {
	public:
		LibraryWidget(Library* lib, Player* player) {
			mLibrary = (lib);
			mPlayer = (player);

			// updateTracks();

			mCurrentTrackInfo.mPlayer = mPlayer;

			setCenterWidget(&mSongList);
			dockWidget(&mCurrentTrackInfo, DockLayout::RIGHT);

			// this->mChildWidgets.pushBack(&mCurrentTrackInfo);
			// this->mChildWidgets.pushBack(&mCurrentTrack);
		}

		void updateTracks() {
			mTracks.clear();

			for (auto track : mTracks) {
				delete track.data();
			}

			for (auto track : mLibrary->mTraks) {
				mTracks.append(new TrackWidget(&track.data()));
			}
		}

		void process(const EventHandler& events) override {
			filter();

			for (auto track : mSongList.getContainer()->getChildren()) {
				if (auto trackWidget = dynamic_cast<TrackWidget*>(track.data())) {
					if (trackWidget->mState == TrackWidget::SELECTED) {
						mCurrentTrackInfo.mTrack = trackWidget->mTrack;
					}
				}
			}

			// mCurrentTrack.proc(events, this->mArea, mSplitView.getFirst());

			DockWidget::process(events);
		}

		// void eventDraw(Canvas& canvas) override {
			// canvas.rect(this->mArea, { 0.1f, 0.1f, 0.1f, 1.f });
		// }

		void filter() {
			if (!mCurrentTrackInfo.isSongFilterChanged) return;

			mSongList.getContainer()->clear();

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

				mSongList.getContainer()->addChild(track.data());
			}

			mCurrentTrackInfo.isSongFilterChanged = false;
		}

	private:
		Library* mLibrary = nullptr;
		Player* mPlayer = nullptr;

		Buffer<TrackWidget*> mTracks;

		ScrollableWidget mSongList;
		TrackInfoWidget mCurrentTrackInfo;
		TrackWidget mCurrentTrack;
	};
}