#pragma once

#include <optional>

#include "Button.hpp"
#include "CanvasWidget.hpp"
#include "ColorModel.hpp"
#include "Commands.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "Dialog.hpp"
#include "FileBrowser.hpp"
#include "Notice.hpp"
#include "Toggles.hpp"
#include "Collapsible.hpp"
#include "ReorderableList.hpp"
#include "LayerThumbnail.hpp"
#include "ThumbnailProvider.hpp"
#include "LibraryView.hpp"
#include "Session.hpp"
#include "SessionRecordingControl.hpp"
#include "Slider.hpp"
#include "TextField.hpp"
#include "Toggles.hpp"
#include "Views.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tp::sketch2d {

	class IconProvider {
	public:
		virtual ~IconProvider() = default;
		[[nodiscard]] virtual ui::ImageId icon(const std::string& name) const = 0;
	};

	class ColorCompare : public ui::Control {
	public:
		ColorCompare();

		RGBA previous = { 0, 0, 0, 1 };
		RGBA current = { 0, 0, 0, 1 };
		std::function<void()> onRestore;

		[[nodiscard]] RectF previousRect() const;
		[[nodiscard]] RectF currentRect() const;

	protected:
		[[nodiscard]] ui::Extent measureContent(ui::Axis axis, halnf crossLength) const override;
		void onEvent(ui::Event& event) override;
		void paint(ui::Painter& painter) override;
	};

	enum class ColorPage { Wheel, Square, Rgb, Hsv, Hsl, Count };

	class ColorPopup : public ui::Panel {
	public:
		static constexpr halnf kPickerHeight = 190;

		explicit ColorPopup(Brush& brush);

		ui::TabView pages;
		ui::ColorWheel wheel;
		ui::ColorSquare square;
		ui::Widget rgbPage;
		ui::LabeledSlider red{ "Red", 0, 255 };
		ui::LabeledSlider green{ "Green", 0, 255 };
		ui::LabeledSlider blue{ "Blue", 0, 255 };
		ui::Widget hsvPage;
		ui::LabeledSlider hsvHue{ "Hue", 0, 360 };
		ui::LabeledSlider hsvSaturation{ "Saturation", 0, 100 };
		ui::LabeledSlider hsvValue{ "Value", 0, 100 };
		ui::Widget hslPage;
		ui::LabeledSlider hslHue{ "Hue", 0, 360 };
		ui::LabeledSlider hslSaturation{ "Saturation", 0, 100 };
		ui::LabeledSlider hslLightness{ "Lightness", 0, 100 };
		ui::Label historyTitle{ "History" };
		ui::SwatchGrid history;
		ui::Widget swatchHeader;
		ui::Label swatchTitle{ "Swatches" };
		ui::Button addSwatch{ "+" };
		ui::SwatchGrid swatches;
		ui::Label swatchHint{ "Hold a swatch to remove it" };
		ui::Widget footer;
		ColorCompare compare;
		ui::TextField hex;
		ui::Button copy{ "Copy" };
		ui::Checkbox eraserMode{ "Eraser mode" };

		std::function<void(const HueColor&)> onPicked;
		std::function<void()> onCopy;
		std::function<void()> onAddSwatch;
		std::function<void(size_t)> onRemoveSwatch;

		void showHistory(const ColorHistory& history);
		void showSwatches(const SwatchSet& swatches);

		void refresh();
		void setPrimary(const HueColor& color);
		void markPrevious();
		[[nodiscard]] ColorPage page() const { return (ColorPage) pages.currentIndex(); }
		void showPage(ColorPage page) { pages.setCurrentIndex((size_t) page); }

	private:
		void pick(const HueColor& color);
		void showColor();

		Brush& mBrush;
		HueColor mPrimary;
		std::vector<HueColor> mHistoryColors;
		bool mShowing = false;
	};

	class BrushPopup : public ui::Panel {
	public:
		explicit BrushPopup(Brush& brush);

		ui::LabeledSlider size{ "Size", 1, Brush::kMaximumSize };
		ui::LabeledSlider flow{ "Flow" };
		ui::LabeledSlider opacity{ "Opacity" };
		ui::LabeledSlider hardness{ "Hardness" };
		ui::LabeledSlider roundness{ "Roundness", 0.1f, 1, 1 };
		ui::LabeledSlider angle{ "Angle", 0, 360 };
		ui::LabeledSlider spacing{ "Spacing", 0.01f, 2, 0.1f };
		ui::LabeledSlider texture{ "Amount" };
		ui::LabeledSlider grainScale{ "Grain scale", 1, 5, 2 };
		ui::LabeledSlider stabilizer{ "Stabilizer" };
		ui::Checkbox sizePressure{ "Pressure size" };
		ui::Checkbox opacityPressure{ "Pressure opacity" };
		ui::Checkbox flowPressure{ "Pressure flow" };
		ui::Checkbox markerBlend{ "Use marker style blending" };
		ui::LabeledSlider smudgeAmount{ "Smudge", 0, 0.99f, 0.8f };
		ui::LabeledSlider blur{ "Blur", 0, 6, 1 };
		ui::LabeledSlider resaturation{ "Resaturation", 0, 0.01f, 0 };
		ui::Checkbox linearMixing{ "Use linear hack (better color mixing, worse edge blurring)" };

		ui::Collapsible shapeSection{ "Shape" };
		ui::Collapsible textureSection{ "Texture" };
		ui::Collapsible pressureSection{ "Pressure" };
		ui::Collapsible smudgeSection{ "Smudge" };
		ui::Button reset{ "Reset brush" };

		std::function<void()> onReset;

		void refresh();

	private:
		void refreshSummaries();

		Brush& mBrush;
	};

	class BrushPresetButton : public ui::Button {
	public:
		explicit BrushPresetButton(const BrushPreset& preset);

		[[nodiscard]] const BrushPreset& preset() const { return mPreset; }
		[[nodiscard]] std::vector<Vec2F> samplePath(const RectF& box) const;

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(ui::Painter& painter, const RectF& box) override;

	private:
		const BrushPreset& mPreset;
	};

	class BrushListPopup : public ui::Panel {
	public:
		explicit BrushListPopup(Brush& brush);

		void refresh();
		[[nodiscard]] ui::Button& button(const std::string& name);
		[[nodiscard]] const std::vector<std::unique_ptr<BrushPresetButton>>& buttons() const { return mButtons; }

		std::function<void()> onChosen;

	private:
		Brush& mBrush;
		std::vector<std::unique_ptr<BrushPresetButton>> mButtons;
	};

	class LayerRow : public ui::Widget {
	public:
		LayerRow(Document& document, Layer& layer, const IconProvider& icons, const std::function<void()>& onChanged);

		[[nodiscard]] Layer& layer() const { return mLayer; }
		[[nodiscard]] std::string detailText() const;
		void setThumbnail(ui::ImageId image);

		ui::Button visibility;
		ui::Widget thumbnail;
		ui::Button name;
		ui::Label details;
		ui::Button remove{ "x" };

		std::function<void(Layer&)> onRename;

	protected:
		void paint(ui::Painter& painter) override;

	private:
		void select(const ui::Modifiers& modifiers);

		ui::ImageId mThumbnail;
		ui::Widget mText;

		Document& mDocument;
		Layer& mLayer;
	};

	class LayersPopup : public ui::Panel {
	public:
		LayersPopup(Document& document, const IconProvider& icons);

		void refresh();
		[[nodiscard]] const std::vector<std::unique_ptr<LayerRow>>& rows() const { return mRows; }

		ui::Button add{ "+ Add layer" };
		ui::LabeledSlider opacity{ "Opacity" };
		ui::Dropdown blend;

		std::function<void(Layer&)> onRename;
		std::function<void()> onChanged;

		void setThumbnails(ThumbnailProvider* thumbnails) { mThumbnails = thumbnails; }
		void showThrough(ui::PopupButton& button);
		void opened() {
			if (!mThumbnailsStale) return;
			mOpening = true;
			refreshRows();
			mOpening = false;
		}
		[[nodiscard]] bool hasStaleThumbnails() const { return mThumbnailsStale; }
		[[nodiscard]] ui::ReorderableList& list() { return mList; }

	private:
		void changed();
		void refreshRows();
		void moveRow(size_t from, size_t to);

		ThumbnailProvider* mThumbnails = nullptr;
		bool mThumbnailsStale = false;
		bool mOpening = false;
		std::vector<ui::PopupButton*> mHosts;
		std::function<bool()> mShownBy;

		Document& mDocument;
		const IconProvider& mIcons;
		ui::Label mTitle{ "Layers" };
		ui::ReorderableList mList;
		std::vector<std::unique_ptr<LayerRow>> mRows;
	};

	class ToolBar : public ui::Panel {
	public:
		ToolBar(Document& document, Brush& brush, const IconProvider& icons);

		void refreshFromState();

		ui::Button tool;
		ui::Button line;
		ui::PopupButton brushTool;
		ui::PopupButton color;
		ui::PopupButton brushSize;
		ui::Button zoom;
		ui::Button undo;
		ui::Button redo;
		ui::Button select;
		ui::Button view;
		ui::PopupButton layers;

		ColorPopup colorPopup;
		BrushPopup brushPopup;
		BrushListPopup brushListPopup;
		LayersPopup layersPopup;

	private:
		Document& mDocument;
		Brush& mBrush;
		Brush mShownBrush;
		ui::Spacer mSpacer;
	};

	class ToolStrip : public ui::Panel {
	public:
		ToolStrip(Brush& brush, const IconProvider& icons);

		void refreshFromState();
		void resetBrushes();
		[[nodiscard]] ui::Button& button(const std::string& icon);
		void reveal(const std::string& icon);
		[[nodiscard]] ui::ScrollView& scroller() { return mScroller; }

		[[nodiscard]] static bool isPaintingBrush(const Brush& brush);

		std::function<void(CanvasTool)> setCanvasTool;
		std::function<CanvasTool()> canvasTool;

	private:
		ui::Button& addTool(const std::string& icon, const std::function<void()>& action);
		void switchToToolBrush(BrushType tool);

		Brush& mBrush;
		const IconProvider& mIcons;
		Brush mPaintingBrush;
		std::vector<std::pair<std::string, std::unique_ptr<ui::Button>>> mTools;
		ui::ScrollView mScroller{ ui::Axis::Vertical };
		ui::Widget mColumn;
	};

	class TransformBar : public ui::Panel {
	public:
		TransformBar();

		ui::Label title{ "Transform" };
		ui::Button flip{ "Flip" };
		ui::Button flop{ "Flop" };
		ui::Button proportional{ "Prop." };
		ui::Button cancel{ "Cancel" };
		ui::Button apply{ "Apply" };
	};

	class AdjustmentDialog : public ui::Dialog {
	public:
		explicit AdjustmentDialog(Document& document);

		void show(ui::Ui& ui, AdjustmentKind kind, std::function<void(bool)> onClosed);
		[[nodiscard]] Adjustment adjustment() const;
		[[nodiscard]] AdjustmentKind kind() const { return mKind; }

		ui::LabeledSlider first{ "", -1, 1, 0 };
		ui::LabeledSlider second{ "", -1, 1, 0 };
		ui::LabeledSlider third{ "", -1, 1, 0 };

	private:
		void preview();
		void finish(bool apply);

		Document& mDocument;
		AdjustmentKind mKind = AdjustmentKind::BrightnessExposure;
		std::function<void(bool)> mOnClosed;
		bool mOpen = false;
	};

	class HistoryLimitDialog : public ui::Dialog {
	public:
		explicit HistoryLimitDialog(Document& document);

		void show(ui::Ui& ui);

		ui::Label stepsLabel{ "Undo steps" };
		ui::NumberField steps;

	private:
		Document& mDocument;
	};

	class RenameLayerDialog : public ui::Dialog {
	public:
		RenameLayerDialog();

		void show(ui::Ui& ui, const std::string& name, std::function<void(const std::string&)> onRename);

		ui::TextField name;

	private:
		std::function<void(const std::string&)> mOnRename;
	};

	class CanvasBackgroundDialog : public ui::Dialog {
	public:
		CanvasBackgroundDialog();

		void show(
			ui::Ui& ui,
			const CanvasBackground& current,
			std::function<void(const CanvasBackground&)> onPreview,
			std::function<void(const CanvasBackground&, const CanvasBackground&)> onApply
		);
		[[nodiscard]] CanvasBackground chosen() const;

		ui::ColorWheel wheel;
		ui::Switch transparent{ "Transparent" };

	private:
		void preview();
		void restore();

		CanvasBackground mBefore;
		bool mActive = false;
		std::function<void(const CanvasBackground&)> mOnPreview;
		std::function<void(const CanvasBackground&, const CanvasBackground&)> mOnApply;
	};

	class CanvasFrameSizeDialog : public ui::Dialog {
	public:
		CanvasFrameSizeDialog();

		void show(ui::Ui& ui, const CanvasFrame& frame, std::function<void(int4, int4)> onApply);

		ui::Label widthLabel{ "Width" };
		ui::NumberField width{ 1, (halnf) CanvasFrame::kMaximumSide, 500 };
		ui::Label heightLabel{ "Height" };
		ui::NumberField height{ 1, (halnf) CanvasFrame::kMaximumSide, 500 };

	private:
		std::function<void(int4, int4)> mOnApply;
	};

	class CanvasAppearanceDialog : public ui::Dialog {
	public:
		CanvasAppearanceDialog();

		void show(ui::Ui& ui, ViewSettings& settings);

		ui::LabeledSlider boardHue{ "Board hue", 0, 360, 0 };
		ui::LabeledSlider boardSaturation{ "Board saturation", 0, 1, 0 };
		ui::LabeledSlider boardValue{ "Board value", 0, 1, 0.6f };
		ui::Switch shadow{ "Draw shadow around canvas" };
		ui::LabeledSlider viewStiffness{ "View motion stiffness", 0, 1, 0.5f };

	private:
		void apply();

		ViewSettings* mSettings = nullptr;
		ViewSettings mBefore;
	};

	constexpr halnf kRotationStepRadians = 0.2617993878f;
	constexpr halnf kBrushSizeStepPercent = 15.0f;
	constexpr halnf kBrushAmountStep = 0.05f;
	constexpr halnf kBrushAngleStep = 10.0f;

	class Sketch2DRoot : public ui::Panel {
	public:
		Sketch2DRoot(
			Document& document,
			ViewportRenderer& renderer,
			Brush& brush,
			const IconProvider& icons,
			std::string sketchesRoot = SketchLibrary::defaultRoot()
		);

		ui::MenuBar menuBar;
		ToolBar toolBar;
		ToolStrip toolStrip;
		CanvasWidget canvas;

		ui::Menu fileMenu;
		ui::Menu editMenu;
		ui::Menu viewMenu;
		ui::Menu channelMenu;
		ui::Menu cursorMenu;
		ui::Menu canvasMenu;
		ui::Menu layerMenu;
		ui::Menu layerBlendMenu;
		ui::Menu layerOpacityMenu;
		ui::Menu historyMenu;
		ui::Menu brushMenu;
		ui::Menu stabilizerMenu;
		ui::Menu resetMenu;
		ui::Menu adjustMenu;
		ui::Menu brushOpacityMenu;
		ui::Menu colorMenu;
		ui::Menu lightnessMenu;
		ui::Menu pickerMenu;
		ui::Menu bucketMenu;
		ui::Menu gradientMenu;
		ui::Menu moveMenu;
		ui::Menu selectMenu;
		ui::Menu lineMenu;
		ui::Menu symmetryMenu;
		ui::Menu perspectiveMenu;
		ui::Menu wandMenu;
		Document::PixelClip clipboard;
		ui::Menu selectAdjustMenu;
		TransformBar transformBar;
		AdjustmentDialog adjustments;
		ui::Menu adjustImageMenu;
		ColorPair colors;
		ColorHistory colorHistory;
		SwatchSet swatches;
		std::string swatchPath;
		ui::Menu debugMenu;
		ui::Menu helpMenu;
		ui::Menu mainMenu;

		CommandRegistry commands;
		[[nodiscard]] const std::string& lastNotice() const { return mLastNotice; }

		void setSessionRecording(ui::SessionRecordingControl* recording);
		[[nodiscard]] ui::SessionRecordingControl* sessionRecording() const { return mSessionRecording; }
		[[nodiscard]] bool isRecordingSession() const { return mSessionRecording && mSessionRecording->isRecording(); }
		ui::ConfirmDialog confirm;
		HistoryLimitDialog historyLimit;
		RenameLayerDialog renameLayer;
		CanvasBackgroundDialog canvasBackground;
		CanvasAppearanceDialog canvasAppearance;
		CanvasFrameSizeDialog canvasFrameSize;
		ui::Notice notice;
		SketchLibrary sketches;
		LibraryView library;

		void refreshFromState();
		std::function<void()> onStateRefreshed;
		std::function<bool()> isDebugUiShown;
		std::string publicName = "Sketch2D";
		std::string versionText = "development build";
		std::function<void(bool)> showDebugUi;
		std::function<bool()> isDesktopLayout;
		std::function<void(bool)> setDesktopLayout;
		std::function<bool()> isFullScreen;
		std::function<void(bool)> setFullScreen;
		ui::MenuItem& addCommand(ui::Menu& menu, Command command, std::function<std::string()> dynamicText = {});
		void notifyText(const std::string& message) { notify(message); }

	protected:
		void onAttached() override;
		void onEvent(ui::Event& event) override;
		void onTick(halnf deltaMs) override;

	private:
		void defineCommands();
		void defineLayerPropertyCommands();
		void defineLayerStructureCommands();
		void defineLayerSelectionCommands();
		void defineLayerMergeCommands();
		void defineCanvasCommands();
		void notify(const std::string& message);
		void layerChanged(Command command, bool changed, const std::string& mergeKey = {});
		void buildMenus();
		void assembleMainMenu();
		void buildCursorMenu();
		void buildHistoryMenu(ui::Menu& menu);
		void buildBrushMenu();
		void defineBrushCommands();
		void defineColorCommands();
		void buildColorMenu();
		void colorChanged(const std::string& notice = {});
		void pullBrushColor();
		void brushChanged(const std::string& notice = {});
		[[nodiscard]] std::optional<BrushParameter> heldBrushAdjustment() const;
		void setStabilizer(halnf amount);
		void defineSessionCommands();

		std::string mLastNotice;
		ui::SessionRecordingControl* mSessionRecording = nullptr;
		ualni mShownRevision = 0;
		Document& mDocument;
		ViewportRenderer& mRenderer;
		Brush& mBrush;
		ui::Widget mBody;
	};
}
