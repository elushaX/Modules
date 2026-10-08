#pragma once

#include "Shortcut.hpp"

#include <functional>
#include <map>
#include <string>

namespace tp::sketch2d {

	enum class Command : int4 {
		NewInfiniteCanvas = 1000002,
		NewCustomSize = 1000003,
		NewFromClipboard = 1000004,
		Open = 1000015,
		Save = 1000016,
		SaveAs = 1000017,
		Import = 1000018,
		Export = 1000019,
		Close = 1000020,
		Cut = 1000021,
		Copy = 1000022,
		Paste = 1000023,
		Transform = 1000024,
		Clear = 1000025,
		Crop = 1000026,
		FlipHorizontal = 1000027,
		FlipVertical = 1000028,
		Invert = 1000029,
		Desaturate = 1000030,
		ValueToAlpha = 1000031,
		ValueToAlphaInverted = 1000032,
		LockView = 1000033,
		FlipViewHorizontal = 1000034,
		FlipViewVertical = 1000035,
		ResetZoom = 1000038,
		ZoomIn = 1000039,
		ZoomOut = 1000041,
		ResetRotation = 1000044,
		RotateLeft = 1000045,
		RotateRight = 1000046,
		FitOnScreen = 1000047,
		ShowChannel = 1000048,
		ShowGrid = 1000050,
		LockGuides = 1000051,
		DeleteAllGuides = 1000052,
		FullScreen = 1000055,
		SelectAllAsRectangle = 1000068,
		SelectAllAsLasso = 1000069,
		SelectCanvasFrame = 1000070,
		Deselect = 1000071,
		InvertSelection = 1000072,
		CanvasBackground = 1000073,
		CanvasAppearance = 1000074,
		CanvasUnitAndResolution = 1000075,
		InfiniteCanvas = 1000076,
		ClearCanvas = 1000077,
		NewLayer = 1000079,
		MoveLayerUp = 1000080,
		MoveLayerDown = 1000081,
		SelectLayerAbove = 1000082,
		SelectLayerBelow = 1000083,
		SelectAllLayers = 1000084,
		DuplicateLayer = 1000085,
		DeleteLayer = 1000086,
		ClearLayer = 1000087,
		MergeLayers = 1000088,
		MergeAllLayers = 1000089,
		RenameLayer = 1000090,
		ToggleLayerVisible = 1000091,
		ToggleLayerClip = 1000092,
		ToggleLayerLock = 1000093,
		ToggleLayerAlphaLock = 1000094,
		PreviousLayerBlend = 1000095,
		NextLayerBlend = 1000096,
		NormalLayerBlend = 1000097,
		LayerOpacityDecrease = 1000098,
		LayerOpacityIncrease = 1000099,
		LayerOpacity0 = 1000100,
		LayerOpacity10 = 1000101,
		LayerOpacity20 = 1000102,
		LayerOpacity30 = 1000103,
		LayerOpacity40 = 1000104,
		LayerOpacity50 = 1000105,
		LayerOpacity60 = 1000106,
		LayerOpacity70 = 1000107,
		LayerOpacity80 = 1000108,
		LayerOpacity90 = 1000109,
		LayerOpacity100 = 1000110,
		BakeLayerOpacity = 1000111,
		Undo = 1000112,
		Redo = 1000113,
		ClearHistory = 1000114,
		SetHistoryLimit = 1000116,
		ShowHidePanels = 1000117,
		CloseAllPanels = 1000118,
		ToggleConsole = 1000119,
		ToggleConsoleFaint = 1000120,
		ToggleConsoleOpaque = 1000121,
		ShowToolPanel = 1000122,
		ShowLinePanel = 1000124,
		ShowBrushListPanel = 1000125,
		ShowBrushOptionsPanel = 1000126,
		ShowColorPanel = 1000127,
		ShowLayersPanel = 1000128,
		ShowRulers = 1000134,
		ResetAllBrushes = 1000137,
		ToolPaint = 1000151,
		ToolPixel = 1000152,
		ToolEraser = 1000153,
		ToolSmudge = 1000154,
		ToolBucket = 1000155,
		ToolPicker = 1000156,
		ToolRuler = 1000157,
		ToolMove = 1000158,
		ToolMagicWand = 1000159,
		ToolLasso = 1000160,
		ToolRectangleSelect = 1000161,
		ToolGradient = 1000162,
		ToolSlice = 1000163,
		ToolZoom = 1000164,
		ToolRotate = 1000165,
		ToolHand = 1000166,
		ToolExport = 1000167,
		ToolExpose = 1000168,
		ToolBackground = 1000169,
		CanvasFrameSize = 1000170,
		AdjustValue = 1000171,
		AdjustColorBalance = 1000172,
		AdjustHueSaturation = 1000173,
		AdjustColorize = 1000174,
		AdjustAlpha = 1000175,
		FreeTransform = 1000177,
		ToolBrushAngle = 1000178,
		ToolBrushSize = 1000179,
		HoldHand = 1000180,
		HoldZoom = 1000181,
		HoldBrushSize = 1000182,
		HoldBrushFlow = 1000183,
		HoldBrushAngle = 1000184,
		HoldBrushOpacity = 1000185,
		LineNormal = 1000187,
		StabilizerDecrease = 1000188,
		StabilizerIncrease = 1000189,
		LineSmooth = 1000190,
		LineRuler = 1000193,
		LineEllipse = 1000196,
		LinePerspective = 1000197,
		LineSymmetry = 1000198,
		LineIsometric = 1000199,
		SetBrush = 1000201,
		ResetBrush = 1000202,
		SmallerBrush = 1000204,
		BiggerBrush = 1000205,
		BrushAngleDecrease = 1000206,
		BrushAngleIncrease = 1000207,
		BrushFlowDecrease = 1000208,
		BrushFlowIncrease = 1000209,
		BrushOpacityDecrease = 1000210,
		BrushOpacityIncrease = 1000211,
		RecordSession = 1900001,
		StopSessionRecording = 1900002,
		AddSwatch = 1900003,
		ClearColorHistory = 1900004,
		SpreadSelection = 1900005,
		BlurSelection = 1900006,
		FillSelection = 1900007,
		FillUnderneath = 1900008,
		CopyToNewLayer = 1900009,
		ShowSymmetryLine = 1900010,
		MoveSymmetryLine = 1900011,
		LockSymmetryLine = 1900012,
		ResetSymmetryLine = 1900013,
		MovePerspectivePoint = 1900014,
		LockPerspective = 1900015,
		ResetPerspective = 1900016,
		BrushOpacity10 = 1000212,
		BrushOpacity20 = 1000213,
		BrushOpacity30 = 1000214,
		BrushOpacity40 = 1000215,
		BrushOpacity50 = 1000216,
		BrushOpacity60 = 1000217,
		BrushOpacity70 = 1000218,
		BrushOpacity80 = 1000219,
		BrushOpacity90 = 1000220,
		BrushOpacity100 = 1000221,
		DefaultColors = 1000229,
		SwapColors = 1000230,
		CopyColor = 1000231,
		PasteColor = 1000232,
		Cooler = 1000233,
		Warmer = 1000234,
		HueComplement = 1000235,
		HueRotateLeft = 1000236,
		HueRotateRight = 1000237,
		Grayer = 1000238,
		Purer = 1000239,
		Darker = 1000240,
		Brighter = 1000241,
		Lightness0 = 1000242,
		Lightness10 = 1000243,
		Lightness20 = 1000244,
		Lightness30 = 1000245,
		Lightness40 = 1000246,
		Lightness50 = 1000247,
		Lightness60 = 1000248,
		Lightness70 = 1000249,
		Lightness80 = 1000250,
		Lightness90 = 1000251,
		Lightness100 = 1000252,
	};

	struct CommandInfo {
		std::string label;
		std::string history;
	};

	[[nodiscard]] const CommandInfo& commandInfo(Command command);

	using ShortcutMap = ui::ShortcutMap<Command>;

	void bindDefaultShortcuts(ShortcutMap& shortcuts);

	class CommandRegistry {
	public:
		using Action = std::function<void(const ui::Event* trigger)>;
		using Predicate = std::function<bool()>;

		CommandRegistry();

		void define(Command command, Action action, Predicate enabled = {}, Predicate checked = {});

		[[nodiscard]] bool isDefined(Command command) const;
		[[nodiscard]] bool isEnabled(Command command) const;
		[[nodiscard]] bool isChecked(Command command) const;

		bool run(Command command, const ui::Event* trigger = nullptr);
		bool handleKey(ui::Event& event);

		[[nodiscard]] ShortcutMap& shortcuts() { return mShortcuts; }
		[[nodiscard]] const ShortcutMap& shortcuts() const { return mShortcuts; }
		[[nodiscard]] std::string shortcutText(Command command) const { return mShortcuts.textFor(command); }

		void setOnCommandRun(std::function<void(Command)> onCommandRun) { mOnCommandRun = std::move(onCommandRun); }

		static constexpr halnf kRepeatWindowMs = 250;
		void advanceClock(halnf deltaMs) { mClockMs += deltaMs; }
		[[nodiscard]] bool isRepeatOf(Command command) const;
		[[nodiscard]] bool wasRecent(Command command) const {
			return mLastCommand == command && mClockMs - mLastRunMs < kRecentWindowMs;
		}
		static constexpr halnf kRecentWindowMs = 250;

	private:
		struct Entry {
			Action action;
			Predicate enabled;
			Predicate checked;
		};

		std::map<Command, Entry> mEntries;
		ShortcutMap mShortcuts;
		std::function<void(Command)> mOnCommandRun;
		halnf mClockMs = 0;
		halnf mLastRunMs = -1.0e9f;
		Command mLastCommand = Command::Undo;
		bool mRunningRepeat = false;
	};
}
