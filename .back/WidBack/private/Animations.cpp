
#include "Animations.hpp"

using namespace tp;

bool AnimValue::gInTransition = false;

halnf AnimValue::interpolate() const {
	if (!mTimeAnim) {
		return mVal;
	}

	auto dt = gCurrentTime - mTimeStart;
	if (dt > mTimeAnim) dt = mTimeAnim;
	auto t = (halnf) dt / (halnf) mTimeAnim;
	t = (halnf) (0.511 + atan((t - 0.36) * 28) / 2.9);
	t = clamp(t, 0.f, 1.f);
	auto out = mValPrev + (mVal - mValPrev) * t;
	return out;
}

AnimValue::AnimValue() {
	mTimeStart = gCurrentTime;
}

void AnimValue::setAnimTime(halni time) { mTimeAnim = time; }

AnimValue::AnimValue(halnf val) {
	mVal = val;
	mValPrev = mVal;
	mTimeStart = gCurrentTime;
}

bool AnimValue::inTransition() const {
	auto timePassed = gCurrentTime - mTimeStart >= mTimeAnim;
	return !timePassed;
}

void AnimValue::set(halnf val) {
	if (!inTransition()) mValPrev = mVal;
	if (val == mVal) return;
	mValPrev = get();
	mVal = val;
	if (!inTransition()) {
		mTimeStart = gCurrentTime;
	}
}

halnf AnimValue::getTarget() const { return mVal; }

void AnimValue::setNoTransition(halnf val) {
	mValPrev = val;
	mVal = val;
	mTimeStart = gCurrentTime - mTimeAnim;
}

halnf AnimValue::get() const {
	if (inTransition()) {
		gInTransition = true;
		return interpolate();
	}
	return mVal;
}

void AnimValue::operator=(halnf val) {
	setNoTransition(val);
}

AnimValue::operator halnf() const { return get(); }

RectF AnimRect::get() const { return { x.get(), y.get(), z.get(), w.get() }; }

RectF AnimRect::getTarget() const { return { x.getTarget(), y.getTarget(), z.getTarget(), w.getTarget() }; }

void AnimRect::setNoTransition(RectF rec) {
	x.setNoTransition(rec.x);
	y.setNoTransition(rec.y);
	z.setNoTransition(rec.z);
	w.setNoTransition(rec.w);
}

void AnimRect::setAnimTime(const halni time) {
	x.setAnimTime(time);
	y.setAnimTime(time);
	z.setAnimTime(time);
	w.setAnimTime(time);
}

void AnimRect::set(const RectF& in) {
	x.set(in.x);
	y.set(in.y);
	z.set(in.z);
	w.set(in.w);
}

AnimColor::AnimColor() : mColor() {}

RGBA AnimColor::get() const {
	auto col = mColor.get();
	return { col.x, col.y, col.z, col.w };
}

void AnimColor::set(const RGBA& col) { mColor.set(RectF(col.r, col.g, col.b, col.a)); }