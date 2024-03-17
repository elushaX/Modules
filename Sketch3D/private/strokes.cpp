
#include "Strokes.h"

using namespace strokes;

Stroke::GLHandles::GLHandles() {
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glGenBuffers(1, &vertexbuffer);
}

void Stroke::GLHandles::sendDataToGPU(tp::Array<Point>* mPoints) {
	glBindVertexArray(VertexArrayID);
	vbo_len = mPoints->length();
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mPoints[0]) * vbo_len, mPoints->buff(), GL_STATIC_DRAW);
}

Stroke::GLHandles::~GLHandles() {
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
}

Stroke::Stroke() {}

tp::Array<Stroke::Point>& Stroke::buff() {
	return mPoints;
}

void Stroke::updateGpuBuffers() {
	mGl.sendDataToGPU(&mPoints);
}

void Stroke::denoisePos(tp::halni passes) {
	for (auto pass : tp::Range(passes)) {
		for (auto pi : tp::Range(mPoints.length() - 2)) {
			mPoints[pi + 1].pos = (mPoints[pi + 1].pos + mPoints[pi].pos + mPoints[pi + 2].pos) / 3.f;
		}
	}
}

void Stroke::denoiseThickness(tp::halni passes) {
	for (auto pass : tp::Range(passes)) {
		for (auto pi : tp::Range(mPoints.length() - 2)) {
			mPoints[pi + 1].thickness = (mPoints[pi].thickness + mPoints[pi + 2].thickness) / 2.f;
		}
	}
}

void Stroke::compress(tp::halnf factor) {
	if (mPoints.length() < 3) {
		return;
	}

	tp::List<Point> passed_poits;

	for (auto idx : tp::Range(mPoints.length())) {
		passed_poits.pushBack(mPoints[idx]);
	}

	tp::ListNode<Point>* min_node = NULL;
	do {
		min_node = NULL;
		tp::halnf min_factor = factor;

		tp::ListNode<Point>* iter = passed_poits.first()->next;
		for (; iter->next; iter = iter->next) {
			tp::vec3f dir1 = (iter->data.pos - iter->prev->data.pos).normalize();
			tp::vec3f dir2 = (iter->next->data.pos - iter->data.pos).normalize();
			tp::halnf factor = 1 - dir1.dot(dir2);

			if (factor < min_factor) {
				min_node = iter;
				min_factor = factor;
			}
		}

		if (min_node) {
			passed_poits.delNode(min_node);
		}
	} while (min_node);


	mPoints.reserve(passed_poits.length());
	for (auto point : passed_poits) {
		mPoints[point.idx()] = point.data();
	}
}

void Stroke::subdiv(tp::halnf precision, tp::Camera* cam, tp::halni passes) {
	// TODO

	if (mPoints.length() < 4) {
		return;
	}

	tp::List<Point> new_points;
	for (auto idx : tp::Range(mPoints.length())) {
		new_points.pushBack(mPoints[idx]);
	}

	auto viewmat = cam->viewmat();
	auto projmat = cam->projmat();

	for (auto i : tp::Range(passes)) {

		auto n_points = new_points.length();

		auto p0 = new_points.first();
		auto p1 = p0->next;
		auto p2 = p1->next;
		auto p3 = p2->next;
	
		while (p3) {

			auto p1_2d = cam->project(p1->data.pos, viewmat, projmat);
			auto p2_2d = cam->project(p2->data.pos, viewmat, projmat);

			auto len = (p1_2d - p2_2d).length();

			if (len > precision * 1.5f) {
				auto const a = (p1->data.pos - p0->data.pos).unitv();
				auto const b = (p2->data.pos - p3->data.pos).unitv();
				auto const l = p1->data.pos - p2->data.pos;
				auto const l_len = l.length();
				auto const ab = a.dot(b);
				auto const la = l.dot(a);
				auto const lb = l.dot(b);
				tp::vec3f mid;

				if (1 - tp::abs(ab) < 0.001f) {
					goto SKIP;
				}

				auto k = (lb - ab * la) / (1 - ab * ab);
				auto t = ab * k - la;
				
				if (k < 0 || t < 0) {
					goto SKIP;
				}

				if (k > l_len) {
					k = l_len;
				}

				if (t > l_len) {
					t = l_len;
				}

				auto const p1_mid = p1->data.pos + (a * t);
				auto const p2_mid = p2->data.pos + (b * k);

				mid = (p1_mid + p2_mid) / 2.f;
				{
					auto const p0 = ((p2->data.pos - p1->data.pos) / 2.f) + p1->data.pos;
					mid = ((mid - p0) / 3.f) + p0;
				}

				auto node = new_points.newNode();
				node->data.pos = mid;
				node->data.thickness = (p1->data.thickness + p2->data.thickness) / 2.f;

				auto insert_after = p1;

				p0 = p1;
				p1 = p2;
				p2 = p3;
				p3 = p3->next;

				new_points.attach(node, insert_after);
				p0 = p0->next;
				continue;
			}

			SKIP:
			p0 = p1;
			p1 = p2;
			p2 = p3;
			p3 = p3->next;
		}

		if (n_points == new_points.length()) {
			break;
		}
	}

	if (new_points.length() != mPoints.length()) {
		mPoints.reserve(new_points.length());
		for (auto point : new_points) {
			mPoints[point.idx()] = point.data();
		}
	}
}

tp::halni Stroke::saveSize() {
	tp::halni out = 0;
	out += sizeof(tp::rgba);
	out += sizeof(tp::alni);
	out += mPoints.length() * sizeof(Point);
	return out;
}

void Stroke::save(tp::File& file) {
	file.write<tp::rgba>(&mCol);

	tp::alni length = mPoints.length();
	file.write<tp::alni>(&length);
	for (auto piter : mPoints) {
		file.write<Point>(&piter.data());
	}
}

void Stroke::load(tp::File& file) {
	tp::rgba color;
	file.read<tp::rgba>(&color);

	tp::alni p_len;
	file.read<tp::alni>(&p_len);

	mPoints.reserve(p_len);
	for (auto piter : mPoints) {
		file.read<Point>(&piter.data());
	}

	updateGpuBuffers();
}

PencilBrush::PencilBrush() {
	mType = "pencil";
}

void PencilBrush::unsureReady(Stroke* stroke, tp::Camera* cam, bool debug) {
	if (stroke->buff().length() == 1) {
		auto new_point = stroke->buff()[0];
		new_point.pos += 0.00001f;
		stroke->buff().pushBack(new_point);
	}

	if (mEnableCompression && !debug) {
		stroke->subdiv(mPrecision, cam, mSubdivPasses);
		stroke->denoisePos(mDenoisePassesPos);
		stroke->compress(mCompressionFactor);
	}

	stroke->updateGpuBuffers();
}

void PencilBrush::sample(Project* proj, tp::vec2f crs, tp::halnf pressure) {
	if (proj->mActiveLayer == -1) {
		return;
	}

	bool max_level = mStroke && mStroke->mPoints.length() > mMaxPoints;
	if (!pressure || max_level) {
		if (mStroke) {
			unsureReady(mStroke, &proj->mCamera);
			proj->mLayers[proj->mActiveLayer]->strokes.pushBack(mStroke);
			mStroke = NULL;
		}

		if (max_level) {
			mStroke = new Stroke();
			mStroke->mCol = mCol;

			mStroke->mPoints.pushBack(proj->mLayers[proj->mActiveLayer]->strokes.last()->data->mPoints.last());
		}

		if (!pressure) {
			return;
		}
	}

	auto thickness = pressure * (proj->mCamera.project({ 0.f, 0.f }) - proj->mCamera.project({ mSize, 0.f })).length();
	bool point_passed = true;

	if (mStroke && mStroke->buff().length()) {
		auto last_point_2d = proj->mCamera.project(mStroke->buff().last().pos);
		point_passed = (crs - last_point_2d).length() > mPrecision;
	}
	
	if (!point_passed) {
		return;
	}

	if (!mStroke) {
		mStroke = new Stroke();
		mStroke->mCol = mCol;
	}

	auto point_coords = proj->mCamera.project(crs);
	mStroke->buff().pushBack({ point_coords, (tp::halnf) thickness });
	mStroke->updateGpuBuffers();
}

void PencilBrush::draw(Renderer* render, tp::Camera* camera) {
	if (mStroke) {
		if (mEnableCompression) {
			mShowStroke.buff() = mStroke->buff();
			mShowStroke.mCol = mStroke->mCol;
			unsureReady(&mShowStroke, camera, 0);
			render->drawStroke(&mShowStroke, camera);
		} else {
			render->drawStroke(mStroke, camera);
		}
	} 
}

PencilBrush::~PencilBrush() {
	if (mStroke) delete mStroke;
}

Project::Layer::~Layer() {
	for (auto str : strokes) {
		delete str.data();
	}
}

Project::Project() {
	mCamera.lookat({ 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.f }, { 0, 0, 1 });

	auto lay = new Layer();
	lay->enabled = true;
	mLayers.pushBack(lay);
	mActiveLayer = 0;

	mBrushes.put("eraser", new EraserBrush());
	mBrushes.put("pencil", new PencilBrush());

	mActiveBrush = "pencil";


	auto vec = mCamera.project({0, 0, 0});
	vec = mCamera.project({-1, 0, 0});
	vec = mCamera.project({1, 0, 0});
}

Project::~Project() {
	for (auto brush : mBrushes) {
		delete brush->val;
	}
}

tp::alni Project::saveSize() {
	tp::alni out = 0;
	out += sizeof(tp::Camera);
	out += sizeof(tp::rgba);
	out += sizeof(tp::halni);

	out += sizeof(tp::alni);
	for (auto layer : mLayers) {
		out += layer.data()->name.save_size();
		out += sizeof(bool);
		out += sizeof(tp::alni);
		for (auto stiter : layer.data()->strokes) {
			out += stiter->saveSize();
		}
	}
	return out;
}

void Project::save(tp::File& file) {
	file.write<tp::Camera>(&mCamera);
	file.write<tp::rgba>(&mBackgroundColor);
	file.write<tp::halni>(&mActiveLayer);

	tp::alni lay_len = mLayers.length();
	file.write<tp::alni>(&lay_len);
	for (auto layer : mLayers) {
		layer.data()->name.save(&file);

		file.write<bool>(&layer.data()->enabled);

		tp::alni len = layer.data()->strokes.length();
		file.write<tp::alni>(&len);
		for (auto stiter : layer.data()->strokes) {
			stiter->save(file);
		}
	}
}

void Project::load(tp::File& file) {
	file.read<tp::Camera>(&mCamera);
	file.read<tp::rgba>(&mBackgroundColor);
	file.read<tp::halni>(&mActiveLayer);

	tp::alni layers_len;
	file.read<tp::alni>(&layers_len);
	mLayers.reserve(layers_len);

	for (tp::alni idx = 0; idx < layers_len; idx++) {

		tp::string key; key.load(&file);
		auto layer = new Layer();
		layer->name = key;
		mLayers[idx] = layer;

		file.read<bool>(&layer->enabled);

		tp::alni len;
		file.read<tp::alni>(&len);

		for (tp::alni str_idx = 0; str_idx < len; str_idx++) {
			auto str = new Stroke();
			layer->strokes.pushBack(str);
			layer->strokes.last()->data->load(file);
		}
	}
}

Renderer::Renderer(tp::vec2f size) : 
	mBuffer(size, { 0.f, 0.f, 1.f, 1.f }, 4),
	mBufferDowncast(size, { 0.f, 0.f, 1.f, 1.f }),
	mShader(".\\rsc\\shaders\\stroke", ".\\rsc\\shaders\\stroke", ".\\rsc\\shaders\\stroke") 
{
	mMatrixUniform = mShader.getu("MVP");
	mColorUniform = mShader.getu("Color");
	mRatioUniform = mShader.getu("Ratio");
	mTargetUniform = mShader.getu("Target");
	mBGColUniform = mShader.getu("BGCol");
}

void Renderer::renderBegin() {
	mBuffer.begin_draw();
	mBuffer.clear();
}

void Renderer::setViewport(tp::rectf viewport) {
	mBuffer.setViewport(viewport);
}

void Renderer::drawStroke(Stroke* str, tp::Camera* camera) {
	//return;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	GLint val;
	glGetIntegerv(GL_SAMPLES, &val);

	glBindVertexArray(str->mGl.VertexArrayID);
	mShader.bind();

	auto cam_mat = camera->transform_mat().transpose();
	glUniformMatrix4fv(mMatrixUniform, 1, GL_FALSE, &cam_mat[0][0]);

	glUniform4fv(mColorUniform, 1, &str->mCol.r);

	glUniform4fv(mBGColUniform, 1, &mBuffer.col_clear.r);

	auto ratio = camera->get_ratio();
	glUniform1fv(mRatioUniform, 1, &ratio);

	auto target = tp::halnf(((camera->get_target() - camera->get_pos()).length() - camera->get_near()) 
		/ (camera->get_far() - camera->get_near()));

	glUniform1fv(mTargetUniform, 1, &target);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, str->mGl.vertexbuffer);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_LINE_STRIP, 0, str->mGl.vbo_len);

	glDisableVertexAttribArray(0);

	mShader.unbind();

}

void Renderer::renderEnd() {
	mBuffer.end_draw();

	mBufferDowncast.begin_draw();

	auto size = mBufferDowncast.getSize();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, mBuffer.buffId());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mBufferDowncast.buffId());
	mBufferDowncast.clear();
	glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	mBufferDowncast.end_draw();
}

tp::uhalni Renderer::getTextudeId() {
	return mBufferDowncast.texId();
}

tp::ogl::fbuffer* Renderer::getBuff() {
	return &mBufferDowncast;
}

void Renderer::setClearCol(tp::rgba col) {
	mBuffer.col_clear = col;
	mBufferDowncast.col_clear = col;
}

Renderer::~Renderer() {
}
