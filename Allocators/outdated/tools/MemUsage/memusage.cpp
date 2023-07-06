

#include "GuiWindow.h"

#include "imgui.h"
#include "implot.h"
#include "nanovg.h"

#include "strings.h"
#include "filesystem.h"
#include "array.h"
#include "list.h"
#include "map.h"

#include "intersections.h"

#include "pickalloc_analizer.h"
#include "pickalloc_cfg.h"

#include <stdio.h>

class Memusage : public tp::GuiWindow {

	tp::string filename;

	static const char logo_len = 16;
	const char logo[logo_len] = "memusage\0\0\0\0\0\0\0";
	char logo_loaded[logo_len];

	PickAllocConfig cfg;

	enum class LoadStatus {
		NONE,
		DONE,
		INVALID_FILE_PATH,
		INVALID_FILE_FORMAT,
		INTERNAL_ERROR,
	} status;

	typedef tp::PickAllocDataAnalizer::Event Event;
	typedef tp::alni SizeKey;

	struct Timeline {
		tp::Array<tp::alnf> size;
		tp::Array<tp::alnf> time;
		tp::ualni peak = 0;
		tp::ualni avereging = 0;

		void recordEvents(tp::Array<Event>& events) {
			tp::ualni total_mem_usage = 0;

			size.extend(events.length());
			time.extend(events.length());

			for (auto ev : events) {

				if (ev->dealloc) {
					total_mem_usage -= ev->size;
				} else {
					total_mem_usage += ev->size;
				}

				time[ev.idx()] = (tp::alnf) ev->time;
				size[ev.idx()] = (tp::alnf) total_mem_usage;

				if (peak < total_mem_usage) {
					peak = total_mem_usage;
				}
			}

			avereging = 1;
		}

		void to_seconds() {
			for (auto tm : time) {
				tm.data() /= 1000;
			}
		}
	};

	// Contains events of fixed size
	struct Group {
		tp::ualni group_size = 0;
		tp::Array<Event> events;

		tp::ualni flag = 0;

		Timeline timeline;

		Group(tp::ualni grp_size) : group_size(grp_size) {}
	};

	tp::HashMap<Group*, SizeKey> groups;
	tp::Array<Group*> groups_sorted;

	public:

	Memusage(const char* filename) : GuiWindow() {

		this->mHeader.mAppTitle = "Mamusage Analizer";
		this->mHeader.mPadding = 10;
		tp::string file_path = __FILE__;
		tp::make_dir_str(file_path.get_writable());
		tp::make_dir_str(file_path.get_writable());
		this->mRscDirectory = file_path + "/";
		this->mGuiRootIsWindow = false;
		LoadFonts();

		this->filename = filename;
	}

	~Memusage() {
		for (auto tl : groups) {
			delete tl->val;
		}

		if (cfg.mAllocationSizes) delete[] cfg.mAllocationSizes;
		if (cfg.mChunkLengths) delete[] cfg.mChunkLengths;
 	}

	void run() {

		// display status
		for (auto i : tp::Range(2)) {
			procInputs();
			beginDraw();
			if (mNativeWindow.mEvents.mRedraw) {
				DrawCallback();
			}
			endDraw();
			mNativeWindow.mEvents.mRedraw = true;
		}

		while (!mNativeWindow.mEvents.mTerminate) {
			tp::Timer timer(5);
			procInputs();
			beginDraw();
			if (mNativeWindow.mEvents.mRedraw) {
				DrawCallback();
			}
			endDraw();
			timer.wait();

			if (status == LoadStatus::NONE) {
				openFile();
				mNativeWindow.mEvents.mRedraw = true;
			}
		}
	}

	private:

	tp::ualni fileContentSize(tp::File& file) {
		return file.size() - fileContentStartIdx();
	}

	tp::ualni fileContentStartIdx() {
		return logo_len;
	}

	void readFile(tp::File& file) {
		using namespace tp;

		// read cfg
		file.read(&cfg.mCapacity);
		cfg.mAllocationSizes = new tp::int4[cfg.mCapacity];
		cfg.mChunkLengths = new tp::int4[cfg.mCapacity];

		file.read_bytes((tp::int1*)cfg.mAllocationSizes, sizeof(tp::int4) * cfg.mCapacity);
		file.read_bytes((tp::int1*)cfg.mChunkLengths, sizeof(tp::int4) * cfg.mCapacity);

		// read evens
		tp::ualni events_length;
		file.read(&events_length);

		// one pass to collect data about groups in 'flag'
		auto events_adress = file.adress;
		for (tp::ualni i = 0; i < events_length; i++) {
			Event evnt;
			file.read(&evnt);
			auto idx = groups.presents(evnt.size);
			Group* group = NULL;
			if (idx) {
				group = groups.getSlotVal(idx);
			} else {
				group = new Group(evnt.size);
				groups.put(evnt.size, group);
			}
			group->flag++;
		}
		for (auto grp : groups) {
			grp->val->events.reserve(grp->val->flag);
			grp->val->flag = 0;
		}
		file.adress = events_adress;

		// read actual events
		for (tp::ualni idx = 0; idx < events_length; idx++) {
			Event evnt;
			file.read(&evnt);
			auto grp = groups.get(evnt.size);
			grp->events[grp->flag] = evnt;
			grp->flag++;
		}

		// initialize additional structure for sorting
		groups_sorted.reserve(groups.size());
		for (auto grp : groups) {
			groups_sorted[grp.entry_idx] = grp->val;
		}

		// generate timelines
		for (auto grp : groups_sorted) {
			grp.data()->timeline.recordEvents(grp.data()->events);
			grp.data()->timeline.to_seconds();
		}
	}

	void openFile() {
		using namespace tp;
		File log(filename.cstr(), osfile_openflags::LOAD);

		if (!log.opened) {
			status = LoadStatus::INVALID_FILE_PATH;
			return;
		}

		log.read_bytes(logo_loaded, logo_len);
		if (!tp::memequal(logo_loaded, logo, logo_len)) {
			status = LoadStatus::INVALID_FILE_FORMAT;
			return;
		}

		log.Preload();

		try {
			readFile(log);
			status = LoadStatus::DONE;
		} catch (...) {
			status = LoadStatus::INTERNAL_ERROR;
			return;
		}
	}

	void HeaderDrawCallback() override {
		using namespace ImGui;
		Button("Analize");
		Button("Save To Header File");
	}

	void DrawCallback() {

		auto pos = this->mNativeWindow.mAppearence.mSize / 2;
		if (status != LoadStatus::DONE) {

			nvgFillColor(mNvg, nvgRGB(250, 250, 250));
			nvgFontSize(mNvg, 24);
			nvgTextAlign(mNvg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);

			switch (status) {
				case LoadStatus::NONE:
					nvgText(mNvg, pos.x, pos.y, "Loading...", 0);
					break;

				case LoadStatus::INVALID_FILE_PATH:
					nvgText(mNvg, pos.x, pos.y, "Invalid File Path", 0);
					break;

				case LoadStatus::INVALID_FILE_FORMAT:
					nvgText(mNvg, pos.x, pos.y, "Invalid File Format", 0);
					break;

				case LoadStatus::INTERNAL_ERROR:
					nvgText(mNvg, pos.x, pos.y, "Internal Error", 0);
			}
			return;
		}

		drawEditor();
	}

	enum class GroupSorting {
		LARGER_SIZE,
		SMALLER_SIZE,
		LARGER_TOTAL_SIZE,
		SMALLER_TOTAL_SIZE,
	} sorting = GroupSorting::LARGER_SIZE;

	void sortGroups(GroupSorting type) {
		if (type == sorting) {
			return;
		}

		switch (type) {
			case Memusage::GroupSorting::LARGER_SIZE:
				groups_sorted.sort([](Group* const& i1, Group* const& i2) {
					return i1->group_size > i2->group_size;
				});
				break;
			case Memusage::GroupSorting::SMALLER_SIZE:
				groups_sorted.sort([](Group* const& i1, Group* const& i2) {
					return i1->group_size < i2->group_size;
				});
				break;
			case Memusage::GroupSorting::LARGER_TOTAL_SIZE:
				break;
			case Memusage::GroupSorting::SMALLER_TOTAL_SIZE:
				break;
			default:
				break;
		}

		sorting = type;
	}

	Group* mGroupActive = NULL;

	void groupView() {
		using namespace ImGui;
		int tmp;

		const char* names[] = {
			{"Larger Group Size"},
			{"Smaller Group Size"},
			//{"LARGER_TOTAL_SIZE"},
			//{"SMALER_TOTAL_SIZE"},
		};

		tmp = (int) sorting;
		Combo(" ", &tmp, names, 2);
		Separator();
		sortGroups((GroupSorting) tmp);

		for (auto grp : groups_sorted) {
			if (Selectable(tp::string((tp::alni)grp.data()->group_size).cstr())) {
				mGroupActive = grp.data();
			}
		}
	}

	void drawTimeline(const Timeline& timeline) {
		if (ImPlot::BeginPlot(" Plot ", {-1, -1}, ImPlotFlags_CanvasOnly | ImPlotFlags_AntiAliased)) {
			ImPlot::PlotLine("Memory Usage", timeline.time.buff(), timeline.size.buff(), (tp::halni) timeline.time.length());
			ImPlot::EndPlot();
		}
	}

	void infoView() {
		using namespace ImGui;
		if (!mGroupActive) {
			Text("No Group Selected");
			return;
		}

		Text("Total Events : %i", mGroupActive->events.length());

		Separator();

		drawTimeline(mGroupActive->timeline);
	}

	void drawEditor() {
		using namespace ImGui;

		Begin("Groups View");
		groupView();
		End();

		Begin("Info");
		infoView();
		End();
	}
};

int main(char argc, char* argv[]) {

	if (argc != 2) {
		printf("invalid arguments");
		return 0;
	}

	//ImPlot::

	Memusage::InitializeTypes();

	{
		Memusage app(argv[1]);
		
		ImPlot::CreateContext();
		
		app.run();
		
		ImPlot::DestroyContext();

		printf("internal error");
	}

	Memusage::UnInitializeTypes();

	tp::terminate();
}