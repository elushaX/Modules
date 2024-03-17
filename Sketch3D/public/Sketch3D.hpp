#pragma once

#include "Buffer.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Topology.hpp"
#include "Color.hpp"
#include "Strings.hpp"

#include "GraphicsApi.hpp"
#include "FrameBuffer.hpp"
#include "Shader.hpp"

namespace tp {

	class Renderer;
	class Project;

	class Stroke {

		friend Renderer;

	public:
		struct Point {
			Vec3F pos = { 0, 0, 0 };
			halnf thickness = NULL;
		};

		Buffer<Point> mPoints;
		RGBA mCol;

	private:
		struct GLHandles {
			GLuint VertexArrayID = 0;
			GLuint vertexbuffer = 0;
			GLuint vbo_len = 0;
			GLHandles();
			void sendDataToGPU(Buffer<Point>* mPoints);
			~GLHandles();
		} mGl;

	public:

		Stroke();

		void denoisePos(halni passes);
		void denoiseThickness(halni passes);
		void compress(halnf factor);
		void subdiv(halnf precition, Camera* cam, halni passes = 1);

		void updateGpuBuffers();

		Buffer<Point>& buff();
	};

	class Brush {
	public:
		String mType = "equal";
		Brush() {}
		virtual void sample(Project* proj, Vec2F crs, halnf pressure) {}
		virtual void draw(Renderer* render, Camera* camera) {}
		virtual ~Brush() {}
	};

	class PencilBrush : public Brush {

		halnf mPrecision = 0.001f;

		halni mDenoisePassesPos = 1;
		halni mDenoisePassesThick = 3;
		halnf mCompressionFactor = 0.0001f;
		halni mSubdivPasses = 3;
		bool mEnableCompression = true;

		halni mMaxPoints = 100;

		Stroke* mStroke = NULL;
		Stroke mShowStroke;

		void unsureReady(Stroke* stroke, Camera* cam, bool debug = false);

	public:
		
		RGBA mCol = RGBA(1.0f);
		halnf mSize = 0.01f;

		PencilBrush();
		virtual void sample(Project* proj, Vec2F crs, halnf pressure) override;
		virtual void draw(Renderer* render, Camera* camera) override;
		virtual ~PencilBrush();
	};

	struct EraserBrush : public Brush {
		EraserBrush() { mType = "eraser"; }
		virtual void sample(Project* proj, Vec2F crs, halnf pressure) override {}
		virtual void draw(Renderer* render, Camera* camera) override {}
		virtual ~EraserBrush() {}
	};

	class Project {
		
	public:

		struct Layer {
			String name = "new layer";
			List<Stroke*> strokes;
			bool enabled = true;
			~Layer();
		};

		Buffer<Layer*> mLayers;
		halni mActiveLayer = -1;

		Camera mCamera;
		RGBA mBackgroundColor = { 0.22f, 0.22f, 0.25f, 1.f };

		Map<String, Brush*> mBrushes;
		String mActiveBrush;

		Project();
		~Project();
	};

	class Renderer {
		RenderBuffer mBufferDowncast;
		RenderBuffer mBuffer;
		RenderShader mShader;

		// shader uniforms
		GLuint mMatrixUniform = 0;
		GLuint mColorUniform = 0;
		GLuint mRatioUniform = 0;
		GLuint mTargetUniform = 0;
		GLuint mBGColUniform = 0;

	public:
		Renderer(Vec2F size);

		void renderBegin();
		void setViewport(RectF viewport);
		void drawStroke(Stroke* str, Camera* camera);
		void renderEnd();

		void setClearCol(RGBA col);
		uhalni getTextudeId();
		RenderBuffer* getBuff();

		~Renderer();
	};
};

/*
void Project::save(File& file) {
	file.write<Camera>(&mCamera);
	file.write<rgba>(&mBackgroundColor);
	file.write<halni>(&mActiveLayer);

	alni lay_len = mLayers.length();
	file.write<alni>(&lay_len);
	for (auto layer : mLayers) {
		layer.data()->name.save(&file);

		file.write<bool>(&layer.data()->enabled);

		alni len = layer.data()->strokes.length();
		file.write<alni>(&len);
		for (auto stiter : layer.data()->strokes) {
			stiter->save(file);
		}
	}
}

void Project::load(File& file) {
	file.read<Camera>(&mCamera);
	file.read<rgba>(&mBackgroundColor);
	file.read<halni>(&mActiveLayer);

	alni layers_len;
	file.read<alni>(&layers_len);
	mLayers.reserve(layers_len);

	for (alni idx = 0; idx < layers_len; idx++) {

		string key; key.load(&file);
		auto layer = new Layer();
		layer->name = key;
		mLayers[idx] = layer;

		file.read<bool>(&layer->enabled);

		alni len;
		file.read<alni>(&len);

		for (alni str_idx = 0; str_idx < len; str_idx++) {
			auto str = new Stroke();
			layer->strokes.pushBack(str);
			layer->strokes.last()->data->load(file);
		}
	}
}
*/

/*
			void Stroke::save(File& file) {
				file.write<rgba>(&mCol);

				alni length = mPoints.length();
				file.write<alni>(&length);
				for (auto piter : mPoints) {
					file.write<Point>(&piter.data());
				}
			}

			void Stroke::load(File& file) {
				rgba color;
				file.read<rgba>(&color);

				alni p_len;
				file.read<alni>(&p_len);

				mPoints.reserve(p_len);
				for (auto piter : mPoints) {
					file.read<Point>(&piter.data());
				}

				updateGpuBuffers();
			}
		*/