
#include "Buffer.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Topology.hpp"
#include "Color.hpp"
#include "Strings.hpp"

namespace strokes {

	class Renderer;
	class Project;

	class Stroke {

		friend Renderer;

	public:
		struct Point {
			tp::vec3f pos;
			tp::halnf thickness = NULL;
		};

		tp::Array<Point> mPoints;
		tp::rgba mCol;

	private:
		struct GLHandles {
			GLuint VertexArrayID = 0;
			GLuint vertexbuffer = 0;
			GLuint vbo_len = 0;
			GLHandles();
			void sendDataToGPU(tp::Array<Point>* mPoints);
			~GLHandles();
		} mGl;

	public:

		Stroke();

		void denoisePos(tp::halni passes);
		void denoiseThickness(tp::halni passes);
		void compress(tp::halnf factor);
		void subdiv(tp::halnf precition, tp::Camera* cam, tp::halni passes = 1);

		void updateGpuBuffers();

		tp::Array<Point>& buff();

		tp::halni saveSize();
		void save(tp::File& file);
		void load(tp::File& file);
	};

	class Brush {
	public:
		tp::string mType = "equal";
		Brush() {}
		virtual void sample(Project* proj, tp::vec2f crs, tp::halnf pressure) {}
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
		
		tp::rgba mCol = tp::rgba(1.0f);
		tp::halnf mSize = 0.01f;

		PencilBrush();
		virtual void sample(Project* proj, tp::vec2f crs, tp::halnf pressure) override;
		virtual void draw(Renderer* render, tp::Camera* camera) override;
		virtual ~PencilBrush();
	};

	struct EraserBrush : public Brush {
		EraserBrush() { mType = "eraser"; }
		virtual void sample(Project* proj, tp::vec2f crs, tp::halnf pressure) override {}
		virtual void draw(Renderer* render, tp::Camera* camera) override {}
		virtual ~EraserBrush() {}
	};

	class Project {
		
	public:

		struct Layer {
			tp::string name = "new layer";
			tp::List<Stroke*> strokes;
			bool enabled = true;
			~Layer();
		};

		tp::Array<Layer*> mLayers;
		tp::halni mActiveLayer = -1;

		tp::Camera mCamera;
		tp::rgba mBackgroundColor = { 0.22f, 0.22f, 0.25f, 1.f };

		tp::HashMap<Brush*, tp::string> mBrushes;
		tp::string mActiveBrush;

		Project();
		~Project();

		tp::alni saveSize();
		void save(tp::File& file);
		void load(tp::File& file);
	};

	class Renderer {
		tp::ogl::fbuffer mBufferDowncast;
		tp::ogl::fbuffer mBuffer;
		tp::ogl::shader mShader;

		// shader uniforms
		GLuint mMatrixUniform = 0;
		GLuint mColorUniform = 0;
		GLuint mRatioUniform = 0;
		GLuint mTargetUniform = 0;
		GLuint mBGColUniform = 0;

	public:
		Renderer(tp::vec2f size);

		void renderBegin();
		void setViewport(tp::rectf viewport);
		void drawStroke(Stroke* str, tp::Camera* camera);
		void renderEnd();

		void setClearCol(tp::rgba col);
		tp::uhalni getTextudeId();
		tp::ogl::fbuffer* getBuff();

		~Renderer();
	};
};