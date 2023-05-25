
#include "memleaks.h"

using namespace tp;

void MemLeaksData::Frame::dfs(Frame& frame, void (*exec)(Frame& frame, void* custom), void* custom) {
	if (frame.flag == Frame::Flags::INUSE) { return; }
	frame.flag = Frame::Flags::INUSE;
	exec(frame, custom);
	for (auto call_info : frame.callers) {
		dfs(*call_info->val.caller, exec, custom);
	}
	frame.flag = Frame::Flags::NONE;
}

MemLeaksData::MemLeaksData(tp::string afilename) {
	filename = afilename;
	load_leaks();

	rootframe.name = "root";
}

void MemLeaksData::increase_caller_count(Frame& frame, FrameId caller_id) {
	auto idx = frame.callers.presents(caller_id);
	if (idx) {
		frame.callers.getSlotVal(idx).count++;
	}
	else {
		Frame* caller_frame = &frames.get(caller_id);
		frame.callers.put(caller_id, { caller_frame, 1 });
	}
}

void MemLeaksData::calc_max_level(Frame& frame, tp::alni& max_level) {
	frame.flag = Frame::Flags::INUSE;

	for (auto call_info : frame.callers) {
		auto caller = call_info->val.caller;
		if (caller->flag != Frame::Flags::INUSE) {
			if (caller->depth_level < frame.depth_level + 1) {
				caller->depth_level = frame.depth_level + 1;
				max_level = MAX(caller->depth_level, max_level);
				calc_max_level(*call_info->val.caller, max_level);
			}
		}
	}

	frame.flag = Frame::Flags::NONE;
}

void MemLeaksData::count_level_users(Frame& frame, tp::alni& max_level, tp::Array<LevelInfo>& levels) {
	if (frame.flag2 == 1) {
		return;
	}
	frame.flag2 = 1;
	levels[frame.depth_level].count_users += 1;
	for (auto call_info : frame.callers) {
		count_level_users(*call_info->val.caller, max_level, levels);
	}
}

void MemLeaksData::apply_position(Frame& frame, tp::Array<LevelInfo>& levels) {
	if (frame.flag2 == 1) {
		return;
	}
	frame.flag2 = 1;


	auto x = -(levels[frame.depth_level].count_users - 1) * sapacing.x / 2 + sapacing.x * levels[frame.depth_level].used_idx;
	frame.tree_view_pos.assign(x, frame.depth_level * sapacing.y);
	levels[frame.depth_level].used_idx++;

	for (auto call_info : frame.callers) {
		apply_position(*call_info->val.caller, levels);
	}
};

void MemLeaksData::construct_tree() {
	using namespace tp;

	// construct tree
	for (auto leak : leaks) {
		increase_caller_count(rootframe, leak.data()[0]);
		for (auto i : Range<alni>(leak->length() - 1)) {
			increase_caller_count(frames.get(leak.data()[i]), leak.data()[i + 1]);
		}
	}

	tp::alni max_depth_level = 0;
	calc_max_level(rootframe, max_depth_level);

	levels.reserve(max_depth_level + 1);

	count_level_users(rootframe, max_depth_level, levels);
	Frame::dfs(rootframe, [](Frame& frame, void*) { frame.flag2 = 0; });

	apply_position(rootframe, levels);
	Frame::dfs(rootframe, [](Frame& frame, void*) { frame.flag2 = 0; });
}

void MemLeaksData::make_connections() {
	tp::halni len = 0;
	for (auto& frame : frames) {
		len += frame.iter->val.callers.size();
	}
	mConnections.reserve(len);
	tp::halni idx = 0;
	for (auto& frame : frames) {
		for (auto& caller : frame.iter->val.callers) {
			caller.iter->val.caller;
			mConnections[idx] = { caller.iter->val.caller, &frame.iter->val, caller.iter->val.count };
			idx++;
		}
	}
}

void MemLeaksData::load_leaks() {
	using namespace tp;

	File log(filename.cstr(), osfile_openflags::LOAD);

	if (!log.opened) {
		status = LoadStatus::INVALID_FILE_PATH;
		return;
	}

	log.Preload();

	const char logo[] = "memleaks\0";
	const char logo_len = 10;
	char logo_loaded[logo_len];

	log.read_bytes(logo_loaded, logo_len);

	if (!tp::memequal(logo_loaded, logo, logo_len)) {
		status = LoadStatus::INVALID_FILE_FORMAT;
		return;
	}

	try {
		alni leaks_len;
		log.read<alni>(&leaks_len);

		leaks.reserve(leaks_len);

		Frame frame;

		for (alni idx = 0; idx < leaks_len; idx++) {

			tp::alni frames_len;
			log.read<tp::alni>(&frames_len);

			leaks[idx].reserve(frames_len);

			for (alni frame_idx = 0; frame_idx < frames_len; frame_idx++) {

				FrameId id;


				log.read<tp::ualni>(&id);
				leaks[idx][frame_idx] = id;

				//if (frames.presents(id)) {
					//continue;
				//}

				frame.name.load(&log);
				frame.file.load(&log);
				log.read<tp::ualni>(&frame.line);

				frame.id = id;

				frames.put(id, frame);
			}
		}

		construct_tree();
		make_connections();

	}
	catch (...) {
		status = LoadStatus::INTERNAL_ERROR;
		return;
	}

	status = LoadStatus::DONE;
}


// -------------------------  Tree View Drawer -------------------------------------- //

void MemLeaksTreeView::setTarget(MemLeaksData* aLeaks) {
	leaks = aLeaks;
	selected_node = NULL;
}

void MemLeaksTreeView::proc() {
	if (!leaks) {
		return;
	}

	// handle selection
	if (mouse_down) {
		selected_node = NULL;

		void (*find_selected)(Frame & frame, void* vec) = [](Frame& frame, void* self_ptr) {
			auto self = (MemLeaksTreeView*)self_ptr;

			auto node_rec = self->nodeBoundsScaled(frame);
			//auto vieport = self->rect;

			if (node_rec.inside(self->vieport_crs)) {
				self->selected_node = &frame;
			}
		};

		Frame::dfs(leaks->rootframe, find_selected, this);
	}

	// handle mouse drag
	if (vieport_crs_delta != 0.f && rect.inside(vieport_crs) && mouse_hold) {
		if (selected_node) {
			selected_node->tree_view_pos -= vieport_crs_delta / (tp::halnf)scaleval;
		}
		else {
			tree_view_pos -= vieport_crs_delta / (tp::halnf)scaleval;
		}
	}

	// calc scale fac
	{
		// zoom_factor :
		// 0 - 3 nodes visiable
		// 1 - all nodes visiable

		auto max_tree_size = MAX(tree_size.x, tree_size.y);
		auto max_node_size = MAX(node_size.x, node_size.y);
		auto min_view_size = MIN(rect.z, rect.w) - 100;

		auto max = min_view_size / max_tree_size;
		auto min = min_view_size / max_node_size;

		scaleval = (max - min) * zoom_factor + min;
	}

	// calc tree size
	void (*execf)(Frame & frame, void* vec) = [](Frame& frame, void* vecp) {
		auto tree_min_max_pos = (tp::rectf*)vecp;
		// if smaller than min
		if (tree_min_max_pos->v1.x > frame.tree_view_pos.x) {
			tree_min_max_pos->v1.x = frame.tree_view_pos.x;
		}
		if (tree_min_max_pos->v1.y > frame.tree_view_pos.y) {
			tree_min_max_pos->v1.y = frame.tree_view_pos.y;
		}
		// if bigger than max
		if (tree_min_max_pos->v2.x < frame.tree_view_pos.x) {
			tree_min_max_pos->v2.x = frame.tree_view_pos.x;
		}
		if (tree_min_max_pos->v2.y < frame.tree_view_pos.y) {
			tree_min_max_pos->v2.y = frame.tree_view_pos.y;
		}
	};

	tree_size = 0;
	tp::rectf tree_min_max_pos = { FLT_MAX, FLT_MIN };
	Frame::dfs(leaks->rootframe, execf, &tree_min_max_pos);
	tree_size = tree_min_max_pos.v2 - tree_min_max_pos.v1;
	tree_size += node_size;
}

void MemLeaksTreeView::draw(tp::glw::Canvas* drawer) {

	drawer->setCol1(col.bg);
	drawer->rect(rect, 2.f);

	drawer->setCol1(col.text);
	if (!leaks) {
		drawer->text("Not Loaded", rect, 5);
		return;
	}

	switch (leaks->status) {
	case MemLeaksData::LoadStatus::INVALID_FILE_PATH:
		drawer->text("INVALID_FILE_PATH", rect, 5);
		return;
	case MemLeaksData::LoadStatus::INVALID_FILE_FORMAT:
		drawer->text("INVALID_FILE_FORMAT", rect, 5);
		return;
	case MemLeaksData::LoadStatus::INTERNAL_ERROR:
		drawer->text("INTERNAL_ERROR", rect, 5);
		return;
	}

	for (auto& frame : leaks->frames) {
		drawNode(frame.iter->val, drawer);
	}

	for (auto& connection : leaks->mConnections) {
		drawConnection(*connection.data().caller, *connection.data().target, connection.data().count, drawer);
	}

	if (selected_node) {
		auto flag = tp::glw::Canvas::Align(tp::glw::Canvas::LEFT | tp::glw::Canvas::TOP);
		tp::rectf info_rec = rect;
		info_rec.pos.x += 5;
		info_rec.pos.y += 25;
		drawer->text(selected_node->name, info_rec, 4, flag);
		info_rec.pos.y += 10;
		drawer->text(selected_node->file, info_rec, 4, flag);
		info_rec.pos.y += 10;
		drawer->text(tp::halni(selected_node->line), info_rec, 4, flag);
	}
}

void MemLeaksTreeView::drawNode(Frame& node, tp::glw::Canvas* drawer) {
	auto rec = nodeBoundsScaled(node);

	// outside
	if (!rec.overlap(rect)) {
		node.flag = Frame::Flags::NONE;
		return;
	}

	rec.clamp(rect);

	drawer->setCol1((&node == selected_node) ? col.node_active : col.node);
	drawer->setCol2(col.node_outline);
	drawer->rect(rec, node_rounding, outline_size);

	drawer->setCol1(col.text);
	auto prev = drawer->mClamping;
	rec.pos.x += 2;
	rec.size.x -= 4;
	drawer->setClamping(rec);
	drawer->text(node.name, rec, text_size, tp::glw::Canvas::LEFT_MIDDLE);
	drawer->setClamping(prev);
}

void MemLeaksTreeView::drawConnection(Frame& from, Frame& to, tp::halni call_count, tp::glw::Canvas* drawer) {

	node_size *= 1.1;
	auto rec1 = nodeBoundsScaled(from);
	auto rec2 = nodeBoundsScaled(to);
	node_size /= 1.1;

	auto c1 = scalePoint(from.tree_view_pos + tree_view_pos);
	auto c2 = scalePoint(to.tree_view_pos + tree_view_pos);

	rec1.clamp_outside(c1, c2);
	rec2.clamp_outside(c1, c2);

	auto viewport = rect;

	if (!viewport.clamp_inside(c1, c2)) {
		return;
	}

	auto dir = (c2 - c1).unitv();
	auto side = dir.normal() * arrow_size * scaleval;

	auto ab = c2 - dir * arrow_size * scaleval;
	auto ae = c2;
	auto al = ab + side;
	auto ar = ab - side;

	if (&from == selected_node || &to == selected_node) {
		//col = ImColor(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
	}

	drawer->setCol1(col.arrow);

	drawer->trig({ ae.x, ae.y }, { al.x, al.y }, { ar.x, ar.y });
	drawer->line({ c1.x, c1.y }, { c2.x, c2.y }, line_thik);

	if (&from == selected_node || &to == selected_node) {
		auto text_pos = c1 + (dir * (tp::halnf)(c1 - c2).length() * 0.8);
		tp::string count = call_count;
		drawer->setCol1(col.text);
		drawer->text(count.cstr(), { text_pos.x, text_pos.y, 0, 0 }, text_size);
	}
}

tp::vec2f MemLeaksTreeView::scalePoint(const tp::vec2f in) {
	return in * (tp::halnf)scaleval + rect.pos + rect.size / 2;
}

tp::rectf MemLeaksTreeView::nodeBounds(Frame& node) {
	tp::rectf out;
	out.pos = node.tree_view_pos + tree_view_pos;
	out.pos -= node_size / 2;
	out.size = node_size;
	return out;
}

tp::rectf MemLeaksTreeView::nodeBoundsScaled(Frame& node) {
	auto out = nodeBounds(node);
	auto p1 = scalePoint(out.pos);
	auto p3 = scalePoint(out.p3());
	out.pos = p1;
	out.size = p3 - p1;
	return out;
}