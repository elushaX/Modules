#include "MediaControls.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Ui.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kTransportSize = 32;
	constexpr halnf kPrimaryTransportSize = 40;
	constexpr halnf kSeekHeight = 30;
	constexpr halnf kSeekTrackThickness = 4;
	constexpr halnf kSeekTimeWidth = 44;
	constexpr halnf kSeekKeyStepSeconds = 5;
	constexpr halnf kVolumeSliderWidth = 96;

	std::vector<Vec2F> triangle(Vec2F center, halnf size, bool pointsRight) {
		const halnf half = size / 2;
		const halnf direction = pointsRight ? 1.0f : -1.0f;
		return { center + Vec2F(-half * 0.8f * direction, -half),
						 center + Vec2F(-half * 0.8f * direction, half),
						 center + Vec2F(half * direction, 0.0f) };
	}
}

std::string tp::ui::formatDuration(halnf seconds) {
	const long total = std::max(0L, (long) std::floor(seconds));
	char text[32];
	if (total >= 3600) std::snprintf(text, sizeof(text), "%ld:%02ld:%02ld", total / 3600, total / 60 % 60, total % 60);
	else std::snprintf(text, sizeof(text), "%ld:%02ld", total / 60, total % 60);
	return text;
}

TransportButton::TransportButton(Glyph glyph, bool primary) :
	Button(""),
	mGlyph(glyph),
	mPrimary(primary) {
	setDebugName("transport button");
	setStyle(Style::Flat);
}

void TransportButton::setGlyph(Glyph glyph) {
	if (glyph == mGlyph) return;
	mGlyph = glyph;
	requestPaint();
}

Extent TransportButton::measureContent(Axis, halnf) const {
	return Extent::rigid(mPrimary ? kPrimaryTransportSize : kTransportSize);
}

RGBA TransportButton::backgroundTarget() const {
	if (!mPrimary) return Button::backgroundTarget();

	const Palette& palette = theme().palette;
	if (!isEffectivelyEnabled()) return palette.control;
	if (isPressed()) return palette.sliderFill;
	if (isHovered()) return palette.toggleOnHovered;
	return palette.toggleOn;
}

void TransportButton::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF box = localRect();
	const halnf radius = box.size.x / 2;

	painter.fillRect(box, background(), radius);
	if (showsFocusRing()) painter.strokeRect(insetRect(box, -2), style.palette.focusRing, radius + 2, 2);

	RGBA glyphColor = mPrimary ? style.palette.onToggle : style.palette.text;
	if (!isEffectivelyEnabled()) glyphColor = style.palette.disabledText;
	paintGlyph(painter, box.center(), box.size.x * 0.36f, glyphColor);
}

void TransportButton::paintGlyph(Painter& painter, Vec2F center, halnf size, const RGBA& color) const {
	const halnf half = size / 2;

	switch (mGlyph) {
		case Glyph::Play: painter.fillPolygon(triangle(center + Vec2F(size * 0.08f, 0.0f), size, true), color); return;

		case Glyph::Pause:
			{
				const halnf bar = size * 0.32f;
				painter.fillRect(makeRect(center.x - half * 0.8f, center.y - half, bar, size), color, bar / 3);
				painter.fillRect(makeRect(center.x + half * 0.8f - bar, center.y - half, bar, size), color, bar / 3);
				return;
			}

		case Glyph::Previous:
		case Glyph::Next:
			{
				const bool next = mGlyph == Glyph::Next;
				const halnf bar = size * 0.16f;
				const halnf barX = next ? center.x + half - bar : center.x - half;
				painter.fillPolygon(triangle(center + Vec2F(next ? -bar : bar, 0.0f), size * 0.85f, next), color);
				painter.fillRect(makeRect(barX, center.y - half, bar, size), color, bar / 2);
				return;
			}

		case Glyph::Volume:
		case Glyph::Muted:
			{
				const halnf body = size * 0.3f;
				const halnf left = center.x - half;
				painter.fillPolygon(
					{ { left, center.y - body / 2 },
						{ left + body, center.y - body / 2 },
						{ left + body * 2, center.y - half },
						{ left + body * 2, center.y + half },
						{ left + body, center.y + body / 2 },
						{ left, center.y + body / 2 } },
					color
				);
				const halnf waveX = left + body * 2.4f;
				if (mGlyph == Glyph::Muted) {
					painter.line({ waveX, center.y - body }, { waveX + body * 1.6f, center.y + body }, color, 1.5f);
					painter.line({ waveX, center.y + body }, { waveX + body * 1.6f, center.y - body }, color, 1.5f);
					return;
				}
				painter.polyline(
					{ { waveX, center.y - body }, { waveX + body * 0.7f, center.y }, { waveX, center.y + body } }, color, 1.5f
				);
				painter.polyline(
					{ { waveX + body * 0.8f, center.y - half * 0.9f },
						{ waveX + body * 1.7f, center.y },
						{ waveX + body * 0.8f, center.y + half * 0.9f } },
					color,
					1.5f
				);
				return;
			}
	}
}

SeekBar::SeekBar() {
	setDebugName("seek bar");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void SeekBar::setDuration(halnf seconds) {
	mDuration = std::max(0.0f, seconds);
	requestPaint();
}

void SeekBar::setPosition(halnf fraction) {
	mPosition = std::clamp(fraction, 0.0f, 1.0f);
	requestPaint();
}

void SeekBar::setBuffered(halnf fraction) {
	mBuffered = std::clamp(fraction, 0.0f, 1.0f);
	requestPaint();
}

Extent SeekBar::measureContent(Axis axis, halnf) const {
	if (axis == Axis::Vertical) return Extent::rigid(kSeekHeight);
	return Extent::shrinkable(kSeekTimeWidth * 2 + 200, kSeekTimeWidth * 2 + 40);
}

RectF SeekBar::trackRect() const {
	const halnf inset = kSeekTimeWidth + theme().metrics.spacing;
	return makeRect(
		inset, (size().y - kSeekTrackThickness) / 2, std::max(0.0f, size().x - inset * 2), kSeekTrackThickness
	);
}

halnf SeekBar::positionAt(halnf localX) const {
	const RectF track = trackRect();
	if (track.size.x <= 0) return 0;
	return std::clamp((localX - track.pos.x) / track.size.x, 0.0f, 1.0f);
}

Vec2F SeekBar::windowPointForPosition(halnf fraction) const {
	const RectF track = trackRect();
	return windowPosition() + Vec2F(track.pos.x + track.size.x * std::clamp(fraction, 0.0f, 1.0f), size().y / 2);
}

void SeekBar::seekBy(halnf seconds) {
	if (mDuration <= 0) return;
	setPosition(mPosition + seconds / mDuration);
	if (mOnSeek) mOnSeek(mPosition);
}

void SeekBar::onEvent(Event& event) {
	if (!isEffectivelyEnabled()) return;

	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary) return;
			mScrubbing = true;
			mScrubPosition = positionAt(event.position().x);
			if (ui()) ui()->capturePointer(*this);
			requestPaint();
			event.accept();
			return;

		case Event::Type::PointerMove:
			if (!mScrubbing) return;
			mScrubPosition = positionAt(event.position().x);
			requestPaint();
			event.accept();
			return;

		case Event::Type::PointerUp:
			if (!mScrubbing || event.button() != PointerButton::Primary) return;
			mScrubbing = false;
			if (ui()) ui()->releasePointer(*this);
			setPosition(mScrubPosition);
			if (mOnSeek) mOnSeek(mPosition);
			event.accept();
			return;

		case Event::Type::KeyDown:
			if (event.keyCode() == InputID::RIGHT) seekBy(kSeekKeyStepSeconds);
			else if (event.keyCode() == InputID::LEFT) seekBy(-kSeekKeyStepSeconds);
			else return;
			event.accept();
			return;

		default: Control::onEvent(event); return;
	}
}

void SeekBar::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF track = trackRect();
	const halnf radius = track.size.y / 2;
	const halnf shown = shownPosition();
	const bool known = mDuration > 0;

	const RectF elapsedBox = makeRect(0, 0, kSeekTimeWidth, size().y);
	const RectF totalBox = makeRect(size().x - kSeekTimeWidth, 0, kSeekTimeWidth, size().y);
	const RGBA timeColor = known ? style.palette.mutedText : style.palette.disabledText;
	painter.text(
		known ? formatDuration(shown * mDuration) : "-:--",
		elapsedBox,
		style.metrics.smallFontSize,
		TextAlign::End,
		timeColor
	);
	painter.text(
		known ? formatDuration(mDuration) : "-:--", totalBox, style.metrics.smallFontSize, TextAlign::Start, timeColor
	);

	painter.fillRect(track, style.palette.track, radius);
	if (mBuffered > 0) {
		painter.fillRect(
			makeRect(track.pos, { track.size.x * mBuffered, track.size.y }), style.palette.controlBorder, radius
		);
	}
	if (shown > 0) {
		painter.fillRect(makeRect(track.pos, { track.size.x * shown, track.size.y }), style.palette.sliderFill, radius);
	}

	if (!known || !(isHovered() || mScrubbing || showsFocusRing())) return;
	const halnf handleRadius = style.metrics.sliderHandleSize / 2;
	const Vec2F handle = { track.pos.x + track.size.x * shown, size().y / 2 };
	if (showsFocusRing()) painter.strokeCircle(handle, handleRadius + 3, style.palette.focusRing, 2);
	painter.fillCircle(handle, handleRadius, style.palette.text);
}

VolumeControl::VolumeControl() {
	setDebugName("volume control");
	setSizing(AxisSizing::fit(), AxisSizing::fit());
	emplaceLayout<RowLayout>(4).setCrossAlign(CrossAlign::Center);

	mMute.setTooltip("Mute");
	mMute.setOnActivate([this]() { setMuted(!mMuted); });
	mSlider.setSizing(AxisSizing::fixed(kVolumeSliderWidth), AxisSizing::fit());
	mSlider.setOnChanged([this](halnf) {
		mMuted = false;
		mMute.setGlyph(TransportButton::Glyph::Volume);
		notify();
	});

	addChild(mMute);
	addChild(mSlider);
}

void VolumeControl::setVolume(halnf volume) { mSlider.setValue(std::clamp(volume, 0.0f, 1.0f)); }

void VolumeControl::setMuted(bool muted) {
	mMuted = muted;
	mMute.setGlyph(muted ? TransportButton::Glyph::Muted : TransportButton::Glyph::Volume);
	mMute.setTooltip(muted ? "Unmute" : "Mute");
	notify();
}

void VolumeControl::notify() {
	if (mOnChanged) mOnChanged(mMuted ? 0.0f : mSlider.value());
}

namespace {
	constexpr halnf kPlayerArtSize = 48;
	constexpr halnf kPlayerSideWidth = 260;
	constexpr halnf kPlayerCenterMaxWidth = 640;
}

PlayerBar::PlayerBar() :
	Panel(Surface::Base) {
	setDebugName("player bar");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	RowLayout& layout = emplaceLayout<RowLayout>(16);
	layout.setPadding(Insets::symmetric(14, 8));
	layout.setCrossAlign(CrossAlign::Center);

	art.setSizing(AxisSizing::fixed(kPlayerArtSize), AxisSizing::fixed(kPlayerArtSize));
	title.setSizing(AxisSizing::fill(), AxisSizing::fit());
	artist.setSizing(AxisSizing::fill(), AxisSizing::fit());
	artist.setTone(Label::Tone::Muted);
	artist.setFontSize(theme().metrics.smallFontSize);

	mText.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mText.emplaceLayout<ColumnLayout>(2);
	mText.addChild(title);
	mText.addChild(artist);

	mInfo.setSizing(AxisSizing::fixed(kPlayerSideWidth), AxisSizing::fit());
	mInfo.emplaceLayout<RowLayout>(10).setCrossAlign(CrossAlign::Center);
	mInfo.addChild(art);
	mInfo.addChild(mText);

	previous.setTooltip("Previous");
	next.setTooltip("Next");
	playPause.setTooltip("Play");
	previous.setOnActivate([this]() {
		if (onPrevious) onPrevious();
	});
	next.setOnActivate([this]() {
		if (onNext) onNext();
	});
	playPause.setOnActivate([this]() {
		if (onPlayPause) onPlayPause();
	});

	mTransport.setSizing(AxisSizing::fit(), AxisSizing::fit());
	mTransport.emplaceLayout<RowLayout>(10).setCrossAlign(CrossAlign::Center);
	mTransport.addChild(previous);
	mTransport.addChild(playPause);
	mTransport.addChild(next);

	mCenter.setSizing(AxisSizing::fill().atMost(kPlayerCenterMaxWidth), AxisSizing::fit());
	ColumnLayout& center = mCenter.emplaceLayout<ColumnLayout>(2);
	center.setCrossAlign(CrossAlign::Center);
	mCenter.addChild(mTransport);
	mCenter.addChild(seek);

	mVolumeColumn.setSizing(AxisSizing::fixed(kPlayerSideWidth), AxisSizing::fit());
	RowLayout& volumeColumn = mVolumeColumn.emplaceLayout<RowLayout>();
	volumeColumn.setMainAlign(MainAlign::End);
	volumeColumn.setCrossAlign(CrossAlign::Center);
	mVolumeColumn.addChild(volume);

	addChild(mInfo);
	addChild(mCenter);
	addChild(mVolumeColumn);
	setTrack(nullptr);
}

void PlayerBar::setTrack(const NowPlaying* track) {
	mHasTrack = track != nullptr;
	title.setText(track ? track->title : "Nothing playing");
	artist.setText(track ? track->artist : "");
	seek.setDuration(track ? track->durationSeconds : 0);
	seek.setEnabled(mHasTrack);
	if (!track) {
		seek.setPosition(0);
		seek.setBuffered(0);
		setPlaying(false);
	}
}

void PlayerBar::setPlaying(bool playing) {
	mPlaying = playing;
	playPause.setGlyph(playing ? TransportButton::Glyph::Pause : TransportButton::Glyph::Play);
	playPause.setTooltip(playing ? "Pause" : "Play");
}
