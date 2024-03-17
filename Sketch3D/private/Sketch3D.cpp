
#include "Sketch3D.hpp"

using namespace tp;

StrokeGPUHandles::StrokeGPUHandles() {
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glGenBuffers(1, &vertexbuffer);
}

void StrokeGPUHandles::sendDataToGPU(Buffer<StrokePoint>* mPoints) {
	glBindVertexArray(VertexArrayID);
	vbo_len = mPoints->size();
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mPoints[0]) * vbo_len, mPoints->getBuff(), GL_DYNAMIC_DRAW);
}

StrokeGPUHandles::~StrokeGPUHandles() {
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
}

Stroke::Stroke() = default;

Buffer<StrokePoint>& Stroke::getPoints() { return mPoints; }

const Buffer<StrokePoint>& Stroke::getPoints() const { return mPoints; }

void Stroke::setColor(const RGBA& col) { mColor = col;	}

const RGBA& Stroke::getColor() const { return mColor; }

void Stroke::updateGpuBuffers() { mGPUHandles.sendDataToGPU(&mPoints); }

void Stroke::denoisePos(halni passes) {
	for (auto pass : Range(passes)) {
		for (auto pi : Range(mPoints.size() - 2)) {
			mPoints[pi + 1].pos = (mPoints[pi + 1].pos + mPoints[pi].pos + mPoints[pi + 2].pos) / 3.f;
		}
	}
}

void Stroke::denoiseThickness(halni passes) {
	for (auto pass : Range(passes)) {
		for (auto pi : Range(mPoints.size() - 2)) {
			mPoints[pi + 1].thickness = (mPoints[pi].thickness + mPoints[pi + 2].thickness) / 2.f;
		}
	}
}

void Stroke::compress(halnf factor) {
	if (mPoints.size() < 3) {
		return;
	}

	List<StrokePoint> passed_poits;

	for (auto idx : Range(mPoints.size())) {
		passed_poits.pushBack(mPoints[idx]);
	}

	List<StrokePoint>::Node* min_node = NULL;
	do {
		min_node = NULL;
		halnf min_factor = factor;

		List<StrokePoint>::Node* iter = passed_poits.first()->next;
		for (; iter->next; iter = iter->next) {
			Vec3F dir1 = (iter->data.pos - iter->prev->data.pos).normalize();
			Vec3F dir2 = (iter->next->data.pos - iter->data.pos).normalize();
			halnf factor = 1 - dir1.dot(dir2);

			if (factor < min_factor) {
				min_node = iter;
				min_factor = factor;
			}
		}

		if (min_node) {
			passed_poits.removeNode(min_node);
		}
	} while (min_node);


	mPoints.reserve(passed_poits.length());

	ualni idx = 0;
	for (auto point : passed_poits) {
		mPoints[idx] = point.data();
		idx++;
	}
}

void Stroke::subdiv(halnf precision, const Camera* cam, halni passes) {
	// TODO

	if (mPoints.size() < 4) {
		return;
	}

	List<StrokePoint> new_points;
	for (auto idx : Range(mPoints.size())) {
		new_points.pushBack(mPoints[idx]);
	}

	auto viewmat = cam->calculateViewMatrix();
	auto projmat = cam->calculateProjectionMatrix();

	for (auto i : Range(passes)) {

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
				auto const a = (p1->data.pos - p0->data.pos).unitV();
				auto const b = (p2->data.pos - p3->data.pos).unitV();
				auto const l = p1->data.pos - p2->data.pos;
				auto const l_len = l.length();
				auto const ab = a.dot(b);
				auto const la = l.dot(a);
				auto const lb = l.dot(b);
				Vec3F mid;

				if (1 - abs(ab) < 0.001f) {
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

	if (new_points.length() != mPoints.size()) {
		mPoints.reserve(new_points.length());
		ualni idx = 0;
		for (auto point : new_points) {
			mPoints[idx] = point.data();
			idx++;
		}
	}
}

PencilBrush::PencilBrush() {
	mType = "pencil";
}

void PencilBrush::ensureReady(Stroke* stroke, const Camera* cam, bool debug) const {
	if (stroke->getPoints().size() == 1) {
		auto new_point = stroke->getPoints()[0];
		new_point.pos += 0.00001f;
		stroke->getPoints().append(new_point);
	}

	if (mEnableCompression && !debug) {
		stroke->subdiv(mPrecision, cam, mSubdivPasses);
		stroke->denoisePos(mDenoisePassesPos);
		stroke->compress(mCompressionFactor);
	}

	stroke->updateGpuBuffers();
}

void PencilBrush::sample(Project* proj, Vec2F crs, halnf pressure) {
	if (proj->mActiveLayer == -1) {
		return;
	}

	bool max_level = mStroke && mStroke->getPoints().size() > mMaxPoints;
	if (!pressure || max_level) {
		if (mStroke) {
			ensureReady(mStroke, &proj->mCamera);
			proj->mLayers[proj->mActiveLayer]->strokes.pushBack(mStroke);
			mStroke = NULL;
		}

		if (max_level) {
			mStroke = new Stroke();
			mStroke->setColor(mCol);

			mStroke->getPoints().append(proj->mLayers[proj->mActiveLayer]->strokes.last()->data->getPoints().last());
		}

		if (!pressure) {
			return;
		}
	}

	auto thickness = pressure * (proj->mCamera.project({ 0.f, 0.f }) - proj->mCamera.project({ mSize, 0.f })).length();
	bool point_passed = true;

	if (mStroke && mStroke->getPoints().size()) {
		auto last_point_2d = proj->mCamera.project(mStroke->getPoints().last().pos);
		point_passed = (crs - last_point_2d).length() > mPrecision;
	}
	
	if (!point_passed) {
		return;
	}

	if (!mStroke) {
		mStroke = new Stroke();
		mStroke->setColor(mCol);
	}

	auto point_coords = proj->mCamera.project(crs);
	mStroke->getPoints().append({ point_coords, (halnf) thickness });
	
	// mStroke->updateGpuBuffers();
}

void PencilBrush::draw(Renderer* render, const Camera* camera) {
	if (mStroke) {
		if (mEnableCompression) {
			mTempDisplayStroke.getPoints() = mStroke->getPoints();
			mTempDisplayStroke.setColor(mStroke->getColor());
			ensureReady(&mTempDisplayStroke, camera, 0);

			render->drawStroke(&mTempDisplayStroke, camera);
		} else {
			render->drawStroke(mStroke, camera);
		}
	}
}

PencilBrush::~PencilBrush() {
	if (mStroke) delete mStroke;
}

Layer::~Layer() {
	for (auto str : strokes) {
		delete str.data();
	}
}

Project::Project() {
	mCamera.lookAtPoint({ 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.f }, { 0, 0, 1 });

	auto lay = new Layer();
	lay->enabled = true;
	mLayers.append(lay);
	mActiveLayer = 0;

	mBrushes.put("eraser", new EraserBrush());
	mBrushes.put("pencil", new PencilBrush());

	mActiveBrush = "pencil";

	Stroke* debug = new Stroke();
	debug->getPoints() = { { { 0, 0, 0 }, 1 }, { { 1, 1, 1 }, 1 } };
	debug->updateGpuBuffers();

	mLayers.last()->strokes.pushBack(debug);

	auto vec = mCamera.project({0, 0, 0});
	vec = mCamera.project({-1, 0, 0});
	vec = mCamera.project({1, 0, 0});
}

void Project::sample(halnf pressure, halnf cameraRatio, Vec2F relativeCameraPos) {
	mCamera.setRatio(cameraRatio);

	auto idx = mBrushes.presents(mActiveBrush);
	if (idx) {
		auto brush = mBrushes.getSlotVal(idx);

		if (brush->mType == "pencil") {
			((tp::PencilBrush*) brush)->mCol = RGBA(1.f);
		}

		brush->sample(this, relativeCameraPos, pressure);
	}
}

Project::~Project() {
	for (auto brush : mBrushes) {
		delete brush->val;
	}
}

Renderer::Renderer(Vec2F size) : 
	mBuffer(size, 4),
	mBufferDowncast(size),
	mShader(".\\rsc\\shaders\\stroke.vert", ".\\rsc\\shaders\\stroke.geom", ".\\rsc\\shaders\\stroke.frag") 
{
	mMaxSize = size;

	mMatrixUniform = mShader.getu("MVP");
	mColorUniform = mShader.getu("Color");
	mRatioUniform = mShader.getu("Ratio");
	mTargetUniform = mShader.getu("Target");
	mBGColUniform = mShader.getu("BGCol");
}

void Renderer::renderToTexture(const Project* project, Vec2F size) {
	size.clamp({ 1, 1 }, mMaxSize);

	setViewport({ 0, 0, size.x, size.y });
	setClearCol(project->mBackgroundColor);
	renderBegin();

	for (auto lay : project->mLayers) {
		if (lay.data()->enabled) {
			for (auto str : lay.data()->strokes) {
				drawStroke(str.data(), &project->mCamera);
			}
		}
	}

	auto idx = project->mBrushes.presents(project->mActiveBrush);
	if (idx) {
		auto brush = project->mBrushes.getSlotVal(idx);
		brush->draw(this, &project->mCamera);
	}

	renderEnd();
}

void Renderer::renderBegin() {
	mBuffer.beginDraw();
	mBuffer.clear();
}

void Renderer::setViewport(RectF viewport) {
	mBuffer.setViewport(viewport);
}

void Renderer::drawStroke(const Stroke* str, const Camera* camera) {
	//return;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	GLint val;
	glGetIntegerv(GL_SAMPLES, &val);

	glBindVertexArray(str->mGPUHandles.VertexArrayID);
	mShader.bind();

	auto cam_mat = camera->calculateTransformationMatrix().transpose();
	glUniformMatrix4fv(mMatrixUniform, 1, GL_FALSE, &cam_mat[0][0]);

	glUniform4fv(mColorUniform, 1, &str->mColor.r);

	glUniform4fv(mBGColUniform, 1, &mBuffer.mClearCol.r);

	auto ratio = camera->getRatio();
	glUniform1fv(mRatioUniform, 1, &ratio);

	auto target = halnf(((camera->getTarget() - camera->getPos()).length() - camera->getNear()) 
		/ (camera->getFar() - camera->getNear()));

	glUniform1fv(mTargetUniform, 1, &target);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, str->mGPUHandles.vertexbuffer);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_LINE_STRIP, 0, str->mGPUHandles.vbo_len);

	glDisableVertexAttribArray(0);

	mShader.unbind();

}

void Renderer::renderEnd() {
	mBuffer.endDraw();

	mBufferDowncast.beginDraw();

	auto size = mBufferDowncast.getSize();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, mBuffer.buffId());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mBufferDowncast.buffId());
	mBufferDowncast.clear();
	glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	mBufferDowncast.endDraw();
}

uhalni Renderer::getTextudeId() {
	return mBufferDowncast.texId();
}

RenderBuffer* Renderer::getBuff() {
	return &mBufferDowncast;
}

void Renderer::setClearCol(RGBA col) {
	mBuffer.mClearCol = col;
	mBufferDowncast.mClearCol = col;
}

Renderer::~Renderer() {
}
