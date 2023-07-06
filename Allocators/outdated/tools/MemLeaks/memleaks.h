
#include "map.h"
#include "array.h"
#include "stringt.h"
#include "rect.h"
#include "color.h"
#include "canvas.h"

namespace tp {

	struct MemLeaksData {

		typedef tp::ualni FrameId;
		typedef tp::Array<FrameId> MemLeak;

		struct Frame;

		struct CallerInfo {
			Frame* caller = NULL;
			tp::ualni count = 0;
		};

		struct Frame {
			tp::string name;
			tp::string file;
			tp::ualni line = 0;
			tp::ualni id = 0;

			tp::HashMap<CallerInfo, FrameId> callers;

			tp::vec2f tree_view_pos = 0;
			bool collapced = true;

			enum class Flags {
				NONE,
				INUSE,
			} flag = Flags::NONE;

			tp::alni flag2 = 0;
			tp::alni depth_level = 0;

			static void dfs(Frame& frame, void (*exec)(Frame& frame, void* custom), void* custom = 0);
		};

		Frame rootframe;
		tp::HashMap<Frame, FrameId> frames;
		tp::Array<MemLeak> leaks;

		struct Connection {
			Frame* caller = NULL;
			Frame* target = NULL;
			tp::ualni count = 0;
		};

		tp::Array<Connection> mConnections;

		tp::string filename;
		tp::vec2f sapacing = { 60.f, 15.f };

		enum class LoadStatus {
			DONE,
			INVALID_FILE_PATH,
			INVALID_FILE_FORMAT,
			INTERNAL_ERROR,
		} status;

		MemLeaksData(tp::string afilename);

	private:

		struct LevelInfo {
			tp::alni count_users;
			tp::alni used_idx;
		};

		tp::Array<LevelInfo> levels;

		void increase_caller_count(Frame& frame, FrameId caller_id);
		void calc_max_level(Frame& frame, tp::alni& max_level);
		void count_level_users(Frame& frame, tp::alni& max_level, tp::Array<LevelInfo>& levels);
		void apply_position(Frame& frame, tp::Array<LevelInfo>& levels);
		void construct_tree();
		void load_leaks();
		void make_connections();
	};

	struct MemLeaksTreeView {

		// inputs
		tp::halnf zoom_factor = 1.f;
		tp::rectf rect = 0.f;
		tp::vec2f vieport_crs = 0.f;
		tp::vec2f vieport_crs_delta = 0.f;
		bool mouse_down = false;
		bool mouse_hold = false;
		bool mouse_up = false;

		// appearence
		struct Colors {
			tp::rgba node = { 0.1f, 0.1f, 0.1f, 1.f };
			tp::rgba node_outline = { 0.5f, 0.5f, 0.5f, 1.f };
			tp::rgba node_active = { 0.2f, 0.2f, 0.2f, 1.f };
			tp::rgba bg = { 0.15f, 0.15f, 0.15f, 1.f };
			tp::rgba text = { 0.9f, 0.9f, 0.9f, 1.f };
			tp::rgba arrow = { 0.5f, 0.5f, 0.5f, 1.f };
		} col;

		tp::halnf text_size = 3;
		tp::halnf arrow_size = 2;
		tp::halnf outline_size = 0.5f;
		tp::halnf line_thik = 0.2f;
		tp::halnf node_rounding = 1;

		void setTarget(MemLeaksData* aLeaks);

		void proc();
		void draw(tp::glw::Canvas* drawer);

	private:
	
		typedef MemLeaksData::Frame Frame;

		MemLeaksData* leaks = NULL;
		Frame* selected_node = NULL;
		bool inside_node = false;
		tp::alnf scaleval = 1.0;
		tp::vec2f tree_size = 0.f;
		tp::vec2f tree_view_pos = { 0.f, -50.f };
		tp::vec2f node_size = { 50, 10 };

		void drawNode(Frame& node, tp::glw::Canvas* drawer);
		void drawConnection(Frame& from, Frame& to, tp::halni call_count, tp::glw::Canvas* drawer);
		tp::vec2f scalePoint(const tp::vec2f in);
		tp::rectf nodeBounds(Frame& node);
		tp::rectf nodeBoundsScaled(Frame& node);
	};
};