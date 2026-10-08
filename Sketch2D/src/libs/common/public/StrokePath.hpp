#pragma once

#include "Vec.hpp"

#include <functional>
#include <vector>

namespace tp::sketch2d {

	class StrokePath {
	public:
		struct Sample {
			Vec2F position = { 0, 0 };
			halnf pressure = 1;
		};

		struct Walk {
			halnf parameter = 0;
			halnf length = 0;
		};

		using Spacing = std::function<halnf(halnf pressure)>;
		using Emit = std::function<void(const Sample& dab)>;

		static constexpr halnf kMinimumSpacing = 0.001f;
		static constexpr int4 kBisectSteps = 24;
		static constexpr halnf kEndTolerance = 0.001f;
		static constexpr int4 kSegmentSteps = 32;
		static constexpr int4 kRefineSteps = 2;

		StrokePath() = default;
		explicit StrokePath(std::vector<Sample> samples);

		void begin(const Sample& first);
		void add(const Sample& sample);
		void advance(Walk& walk, const Spacing& spacing, const Emit& emit, bool finished);
		void advanceTo(Walk& walk, halnf end, const Spacing& spacing, const Emit& emit);

		[[nodiscard]] size_t sampleCount() const { return mSamples.size(); }
		[[nodiscard]] const std::vector<Sample>& samples() const { return mSamples; }
		[[nodiscard]] Sample at(halnf parameter) const;
		[[nodiscard]] halnf endParameter(bool finished) const;
		void forEachPoint(halnf from, halnf to, const std::function<void(Vec2F from, Vec2F to)>& visit) const;

	private:
		[[nodiscard]] halnf parameterAtLength(halnf length) const;
		[[nodiscard]] halnf lengthAt(halnf parameter) const;
		void measureSegments(halnf end);

		std::vector<Sample> mSamples;
		std::vector<halnf> mLengths = { 0.0f };
	};
}
