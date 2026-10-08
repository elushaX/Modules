#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Control.hpp"
#include "Label.hpp"
#include "Slider.hpp"

#include <functional>
#include <string>

namespace tp::ui {

	class TransportButton : public Button {
	public:
		enum class Glyph { Play, Pause, Previous, Next, Volume, Muted };

		explicit TransportButton(Glyph glyph, bool primary = false);

		void setGlyph(Glyph glyph);
		[[nodiscard]] Glyph glyph() const { return mGlyph; }
		[[nodiscard]] bool isPrimary() const { return mPrimary; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		void paintGlyph(Painter& painter, Vec2F center, halnf size, const RGBA& color) const;

		Glyph mGlyph;
		bool mPrimary;
	};

	std::string formatDuration(halnf seconds);

	class SeekBar : public Control {
	public:
		SeekBar();

		void setDuration(halnf seconds);
		[[nodiscard]] halnf duration() const { return mDuration; }

		void setPosition(halnf fraction);
		[[nodiscard]] halnf position() const { return mPosition; }

		void setBuffered(halnf fraction);
		[[nodiscard]] halnf buffered() const { return mBuffered; }

		[[nodiscard]] bool isScrubbing() const { return mScrubbing; }
		[[nodiscard]] halnf shownPosition() const { return mScrubbing ? mScrubPosition : mPosition; }
		[[nodiscard]] Vec2F windowPointForPosition(halnf fraction) const;

		void setOnSeek(std::function<void(halnf)> onSeek) { mOnSeek = std::move(onSeek); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;

	private:
		[[nodiscard]] RectF trackRect() const;
		[[nodiscard]] halnf positionAt(halnf localX) const;
		void seekBy(halnf seconds);

		halnf mDuration = 0;
		halnf mPosition = 0;
		halnf mBuffered = 0;
		halnf mScrubPosition = 0;
		bool mScrubbing = false;
		std::function<void(halnf)> mOnSeek;
	};

	class VolumeControl : public Widget {
	public:
		VolumeControl();

		void setVolume(halnf volume);
		[[nodiscard]] halnf volume() const { return mSlider.value(); }
		[[nodiscard]] bool isMuted() const { return mMuted; }
		void setMuted(bool muted);

		void setOnChanged(std::function<void(halnf)> onChanged) { mOnChanged = std::move(onChanged); }

		TransportButton& muteButton() { return mMute; }
		Slider& slider() { return mSlider; }

	private:
		void notify();

		TransportButton mMute{ TransportButton::Glyph::Volume };
		Slider mSlider{ 0, 1, 1 };
		bool mMuted = false;
		std::function<void(halnf)> mOnChanged;
	};
}

namespace tp::ui {

	struct NowPlaying {
		std::string title;
		std::string artist;
		std::string album;
		halnf durationSeconds = 0;
		bool loved = false;
	};

	class PlayerBar : public Panel {
	public:
		PlayerBar();

		void setTrack(const NowPlaying* track);
		[[nodiscard]] bool hasTrack() const { return mHasTrack; }

		void setPlaying(bool playing);
		[[nodiscard]] bool isPlaying() const { return mPlaying; }

		void setPosition(halnf fraction) { seek.setPosition(fraction); }
		void setBuffered(halnf fraction) { seek.setBuffered(fraction); }

		std::function<void()> onPlayPause;
		std::function<void()> onPrevious;
		std::function<void()> onNext;

		Panel art{ Surface::Raised };
		Label title;
		Label artist;
		TransportButton previous{ TransportButton::Glyph::Previous };
		TransportButton playPause{ TransportButton::Glyph::Play, true };
		TransportButton next{ TransportButton::Glyph::Next };
		SeekBar seek;
		VolumeControl volume;

	private:
		bool mHasTrack = false;
		bool mPlaying = false;
		Widget mInfo;
		Widget mText;
		Widget mCenter;
		Widget mTransport;
		Widget mVolumeColumn;
	};
}
