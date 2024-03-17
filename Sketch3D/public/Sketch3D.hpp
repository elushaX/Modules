
#include "Buffer.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Topology.hpp"
#include "Color.hpp"
#include "Strings.hpp"

#include "GraphicsApi.hpp"
#include "FrameBuffer.hpp"
#include "Shader.hpp"

namespace strokes {

	class Renderer;
	class Project;

	class Stroke {

		friend Renderer;

	public:
		struct Point {
			tp::Vec3F pos;
			tp::halnf thickness = NULL;
		};

		tp::Buffer<Point> mPoints;
		tp::RGBA mCol;

	private:
		struct GLHandles {
			GLuint VertexArrayID = 0;
			GLuint vertexbuffer = 0;
			GLuint vbo_len = 0;
			GLHandles();
			void sendDataToGPU(tp::Buffer<Point>* mPoints);
			~GLHandles();
		} mGl;

	public:

		Stroke();

		void denoisePos(tp::halni passes);
		void denoiseThickness(tp::halni passes);
		void compress(tp::halnf factor);
		void subdiv(tp::halnf precition, tp::Camera* cam, tp::halni passes = 1);

		void updateGpuBuffers();

		tp::Buffer<Point>& buff();
	};

	class Brush {
	public:
		tp::String mType = "equal";
		Brush() {}
		virtual void sample(Project* proj, tp::Vec2F crs, tp::halnf pressure) {}
		virtual void draw(Renderer* render, tp::Camera* camera) {}
		virtual ~Brush() {}
	};

	class PencilBrush : public Brush {

		tp::halnf mPrecision = 0.001f;

		tp::halni mDenoisePassesPos = 1;
		tp::halni mDenoisePassesThick = 3;
		tp::halnf mCompressionFactor = 0.0001f;
		tp::halni mSubdivPasses = 3;
		bool mEnableCompression = true;

		tp::halni mMaxPoints = 100;

		Stroke* mStroke = NULL;
		Stroke mShowStroke;

		void unsureReady(Stroke* stroke, tp::Camera* cam, bool debug = false);

	public:
		
		tp::RGBA mCol = tp::RGBA(1.0f);
		tp::halnf mSize = 0.01f;

		PencilBrush();
		virtual void sample(Project* proj, tp::Vec2F crs, tp::halnf pressure) override;
		virtual void draw(Renderer* render, tp::Camera* camera) override;
		virtual ~PencilBrush();
	};

	struct EraserBrush : public Brush {
		EraserBrush() { mType = "eraser"; }
		virtual void sample(Project* proj, tp::Vec2F crs, tp::halnf pressure) override {}
		virtual void draw(Renderer* render, tp::Camera* camera) override {}
		virtual ~EraserBrush() {}
	};

	class Project {
		
	public:

		struct Layer {
			tp::String name = "new layer";
			tp::List<Stroke*> strokes;
			bool enabled = true;
			~Layer();
		};

		tp::Buffer<Layer*> mLayers;
		tp::halni mActiveLayer = -1;

		tp::Camera mCamera;
		tp::RGBA mBackgroundColor = { 0.22f, 0.22f, 0.25f, 1.f };

		tp::Map<tp::String, Brush*> mBrushes;
		tp::String mActiveBrush;

		Project();
		~Project();
	};

	class Renderer {
		tp::RenderBuffer mBufferDowncast;
		tp::RenderBuffer mBuffer;
		tp::RenderShader mShader;

		// shader uniforms
		GLuint mMatrixUniform = 0;
		GLuint mColorUniform = 0;
		GLuint mRatioUniform = 0;
		GLuint mTargetUniform = 0;
		GLuint mBGColUniform = 0;

	public:
		Renderer(tp::Vec2F size);

		void renderBegin();
		void setViewport(tp::RectF viewport);
		void drawStroke(Stroke* str, tp::Camera* camera);
		void renderEnd();

		void setClearCol(tp::RGBA col);
		tp::uhalni getTextudeId();
		tp::RenderBuffer* getBuff();

		~Renderer();
	};
};

/*
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
*/

/*
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
		*/