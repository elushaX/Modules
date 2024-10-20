#pragma once

#include "Buffer.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Topology.hpp"
#include "Color.hpp"

#include "GraphicsApi.hpp"
#include "FrameBuffer.hpp"
#include "Shader.hpp"

namespace tp {

	class Renderer;
	class Project;
	class Stroke;
	class Brush;

	struct StrokePoint {
		Vec3F pos = { 0, 0, 0 };
		halnf thickness = 0;
	};

	struct StrokeGPUHandles {
		GLuint VertexArrayID = 0;
		GLuint vertexbuffer = 0;
		GLuint vbo_len = 0;
		StrokeGPUHandles();
		void sendDataToGPU(Buffer<StrokePoint>* mPoints);
		~StrokeGPUHandles();
	};

	class Renderer {
	public:
		Renderer(Vec2F size); // max render size
		~Renderer();

		void renderToTexture(const Project* project, Vec2F size); // needed size

		void renderBegin();
		void setViewport(RectF viewport);
		void drawStroke(const Stroke* str, const Camera* camera);
		void renderEnd();

		void setClearCol(RGBA col);
		uhalni getTextudeId();
		RenderBuffer* getBuff();

	private:
		RenderBuffer mBufferDowncast;
		RenderBuffer mBuffer;
		RenderShader mShader;

		// shader uniforms
		GLuint mMatrixUniform = 0;
		GLuint mColorUniform = 0;
		GLuint mRatioUniform = 0;
		GLuint mTargetUniform = 0;
		GLuint mBGColUniform = 0;

		Vec2F mMaxSize;
	};

	class Stroke {
	public:
		Stroke();
		void denoisePos(halni passes);
		void denoiseThickness(halni passes);
		void compress(halnf factor);
		void subdiv(halnf precition, const Camera* cam, halni passes = 1);
		void updateGpuBuffers();
		Buffer<StrokePoint>& getPoints();
		const Buffer<StrokePoint>& getPoints() const;
		void setColor(const RGBA& col);
		const RGBA& getColor() const;

	private:
		Buffer<StrokePoint> mPoints;
		RGBA mColor;

		friend Renderer;
		StrokeGPUHandles mGPUHandles;
	};

	struct Layer {
		Layer() = default;
		~Layer();

		std::string name = "new layer";
		List<Stroke*> strokes; // TODO use vector
		bool enabled = true;
	};

	class Project {
	public:
		Project();
		~Project();

		void setPencil();
		void setEraser();

		// pos from -1 to 1 (left ot right bottom to top)
		void sample(halnf pressure, halnf cameraRatio, Vec2F relativeCameraPos);

	public:
		Buffer<Layer*> mLayers;
		halni mActiveLayer = -1;

		Camera mCamera;
		RGBA mBackgroundColor = { 0.22f, 0.22f, 0.25f, 1.f };

		Map<std::string, Brush*> mBrushes;
		std::string mActiveBrush;
	};

	class Brush {
	public:
		std::string mType = "equal";
		Brush() = default;
		virtual void sample(Project* proj, Vec2F crs, halnf pressure) {}
		virtual void finish(Project* proj) {}
		virtual void draw(Renderer* render, const Camera* camera) {}
		virtual ~Brush() = default;
	};

	class PencilBrush : public Brush {
	public:
		PencilBrush();
		~PencilBrush() override;

		void finish(Project* proj) override;
		void sample(Project* proj, Vec2F crs, halnf pressure) override;
		void draw(Renderer* render, const Camera* camera) override;

	private:
		void ensureReady(Stroke* stroke, const Camera* cam, bool debug = false) const;

	public:
		RGBA mCol = RGBA(1.0f);
		halnf mSize = 0.01f;

	private:
		halnf mPrecision = 0.001f;

		halni mDenoisePassesPos = 1;
		halni mDenoisePassesThick = 3;
		halnf mCompressionFactor = 0.0001f;
		halni mSubdivPasses = 3;
		bool mEnableCompression = true;

		halni mMaxPoints = 100;

		Stroke* mStroke = nullptr;
		Stroke mTempDisplayStroke;
	};

	struct EraserBrush : public Brush {
		EraserBrush() { mType = "eraser"; }
		void sample(Project* proj, Vec2F crs, halnf pressure) override {}
		void draw(Renderer* render, const Camera* camera) override {}
		~EraserBrush() override = default;
	};
}