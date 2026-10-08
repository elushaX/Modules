#include "StrokePath.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	namespace {
		halnf catmullRom(halnf p0, halnf p1, halnf p2, halnf p3, halnf t) {
			const halnf t2 = t * t;
			const halnf t3 = t2 * t;
			return 0.5f * (2.0f * p1 + (p2 - p0) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
										 (3.0f * p1 - p0 - 3.0f * p2 + p3) * t3);
		}

		halnf distance(Vec2F a, Vec2F b) {
			const Vec2F d = b - a;
			return std::sqrt(d.x * d.x + d.y * d.y);
		}
	}

	StrokePath::StrokePath(std::vector<Sample> samples) :
		mSamples(std::move(samples)) {}

	void StrokePath::begin(const Sample& first) {
		mSamples.clear();
		mSamples.push_back(first);
		mLengths.assign(1, 0.0f);
	}

	void StrokePath::add(const Sample& sample) {
		if (!mSamples.empty() && distance(mSamples.back().position, sample.position) <= 0.0f) {
			mSamples.back().pressure = sample.pressure;
			return;
		}
		mSamples.push_back(sample);
	}

	StrokePath::Sample StrokePath::at(halnf parameter) const {
		if (mSamples.empty()) return {};
		const int4 last = (int4) mSamples.size() - 1;
		parameter = std::clamp(parameter, 0.0f, (halnf) last);
		const int4 index = std::min((int4) std::floor(parameter), std::max(last - 1, 0));
		const halnf t = parameter - (halnf) index;
		const Sample& p1 = mSamples[(size_t) index];
		const Sample& p2 = mSamples[(size_t) std::min(index + 1, last)];
		const Sample& p0 = mSamples[(size_t) std::max(index - 1, 0)];
		const Sample& p3 = mSamples[(size_t) std::min(index + 2, last)];
		Sample result;
		result.position = { catmullRom(p0.position.x, p1.position.x, p2.position.x, p3.position.x, t),
												catmullRom(p0.position.y, p1.position.y, p2.position.y, p3.position.y, t) };
		result.pressure = std::clamp(p1.pressure + (p2.pressure - p1.pressure) * t, 0.0f, 1.0f);
		return result;
	}

	halnf StrokePath::endParameter(bool finished) const {
		const halnf last = (halnf) mSamples.size() - 1.0f;
		return finished ? last : std::max(0.0f, last - 1.0f);
	}

	void StrokePath::measureSegments(halnf end) {
		const size_t needed = (size_t) std::ceil(end) * (size_t) kSegmentSteps + 1;
		while (mLengths.size() < needed) {
			const size_t step = mLengths.size();
			const halnf from = (halnf) (step - 1) / (halnf) kSegmentSteps;
			const halnf to = (halnf) step / (halnf) kSegmentSteps;
			mLengths.push_back(mLengths.back() + distance(at(from).position, at(to).position));
		}
	}

	halnf StrokePath::lengthAt(halnf parameter) const {
		const halnf scaled = parameter * (halnf) kSegmentSteps;
		const size_t index = std::min((size_t) scaled, mLengths.size() - 2);
		const halnf t = scaled - (halnf) index;
		return mLengths[index] + (mLengths[index + 1] - mLengths[index]) * t;
	}

	halnf StrokePath::parameterAtLength(halnf length) const {
		const auto upper = std::lower_bound(mLengths.begin(), mLengths.end(), length);
		if (upper == mLengths.begin()) return 0;
		if (upper == mLengths.end()) return (halnf) (mLengths.size() - 1) / (halnf) kSegmentSteps;
		const size_t index = (size_t) (upper - mLengths.begin()) - 1;
		const halnf span = mLengths[index + 1] - mLengths[index];
		halnf t = span > 0.0f ? (length - mLengths[index]) / span : 0.0f;
		const halnf start = (halnf) index / (halnf) kSegmentSteps;
		const halnf width = 1.0f / (halnf) kSegmentSteps;
		const Vec2F origin = at(start).position;
		const halnf wanted = length - mLengths[index];
		for (int4 iteration = 0; iteration < kRefineSteps && span > 0.0f; iteration++) {
			const halnf reached = distance(origin, at(start + t * width).position);
			t = std::clamp(t + (wanted - reached) / span, 0.0f, 1.0f);
		}
		return start + t * width;
	}

	void StrokePath::advance(Walk& walk, const Spacing& spacing, const Emit& emit, bool finished) {
		advanceTo(walk, endParameter(finished), spacing, emit);
	}

	void StrokePath::advanceTo(Walk& walk, halnf end, const Spacing& spacing, const Emit& emit) {
		if (mSamples.size() < 2) return;
		end = std::min(end, endParameter(true));
		if (end <= walk.parameter) return;
		measureSegments(end);
		const halnf endLength = lengthAt(end);
		while (true) {
			const Sample from = at(walk.parameter);
			const halnf step = std::max(spacing(from.pressure), kMinimumSpacing);
			const halnf target = walk.length + step;
			if (target > endLength + step * kEndTolerance) return;
			walk.length = std::min(target, endLength);
			walk.parameter = std::min(parameterAtLength(walk.length), end);
			emit(at(walk.parameter));
		}
	}

	void StrokePath::forEachPoint(halnf from, halnf to, const std::function<void(Vec2F from, Vec2F to)>& visit) const {
		if (mSamples.empty()) return;
		const halnf step = 1.0f / (halnf) kSegmentSteps;
		Vec2F previous = at(from).position;
		for (halnf parameter = std::floor(from * (halnf) kSegmentSteps) * step + step; parameter < to; parameter += step) {
			const Vec2F next = at(parameter).position;
			visit(previous, next);
			previous = next;
		}
		visit(previous, at(to).position);
	}
}
