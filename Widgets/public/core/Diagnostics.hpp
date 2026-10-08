#pragma once

#include "Environment.hpp"
#include "Timing.hpp"

namespace tp::ui {

	class Widget;
	class Event;

	struct FrameStats {
		ualni layoutPasses = 0;
		ualni measuredWidgets = 0;
		ualni arrangedWidgets = 0;
		ualni skippedWidgets = 0;
		ualni deliveredEvents = 0;
		ualni paintedWidgets = 0;

		time_ns inputNs = 0;
		time_ns animationNs = 0;
		time_ns layoutNs = 0;
		time_ns paintNs = 0;
	};

	class UiObserver {
	public:
		virtual ~UiObserver() = default;

		virtual void onEventDelivering(const Widget& receiver, const Event& event) {}
		virtual void onEventDelivered(const Widget& receiver, const Event& event) {}
		virtual void onLayoutPassStarted() {}
		virtual void onWidgetArranged(const Widget& widget) {}
		virtual void onWidgetDetached(const Widget& widget) {}
	};

	struct Diagnostics {
		FrameStats stats;
		UiObserver* observer = nullptr;
	};

	class ScopedNanoseconds {
	public:
		explicit ScopedNanoseconds(time_ns& accumulator) :
			mAccumulator(accumulator),
			mStart(get_time_ns()) {}

		~ScopedNanoseconds() { mAccumulator += get_time_ns() - mStart; }

		ScopedNanoseconds(const ScopedNanoseconds&) = delete;
		ScopedNanoseconds& operator=(const ScopedNanoseconds&) = delete;

	private:
		time_ns& mAccumulator;
		time_ns mStart;
	};
}
