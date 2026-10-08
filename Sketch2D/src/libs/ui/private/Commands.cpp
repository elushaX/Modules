#include "Commands.hpp"

using namespace tp;
using namespace tp::ui;

namespace tp::sketch2d {

	const CommandInfo& commandInfo(Command command) {
		static const std::map<Command, CommandInfo> infos = {
			{ Command::NewInfiniteCanvas, { "Infinite canvas", "" } },
			{ Command::NewCustomSize, { "Custom size", "" } },
			{ Command::NewFromClipboard, { "Clipboard", "" } },
			{ Command::Open, { "Open", "" } },
			{ Command::Save, { "Save", "" } },
			{ Command::SaveAs, { "Save as", "" } },
			{ Command::Import, { "Import", "Import" } },
			{ Command::Export, { "Export", "" } },
			{ Command::Close, { "Close", "" } },
			{ Command::Cut, { "Cut", "Clear pixels" } },
			{ Command::Copy, { "Copy", "" } },
			{ Command::Paste, { "Paste", "Paste image" } },
			{ Command::Transform, { "Transform", "Transform" } },
			{ Command::Clear, { "Clear", "Clear pixels" } },
			{ Command::Crop, { "Crop", "Crop pixels" } },
			{ Command::FlipHorizontal, { "Flip horizontal", "Transform" } },
			{ Command::FlipVertical, { "Flip vertical", "Transform" } },
			{ Command::Invert, { "Invert", "Invert" } },
			{ Command::Desaturate, { "Desaturate", "Desaturate" } },
			{ Command::ValueToAlpha, { "Copy value to alpha", "Value to alpha" } },
			{ Command::ValueToAlphaInverted, { "Copy value to alpha inverted", "Value to alpha inverted" } },
			{ Command::LockView, { "Lock view", "" } },
			{ Command::FlipViewHorizontal, { "Flip horizontal", "" } },
			{ Command::FlipViewVertical, { "Flip vertical", "" } },
			{ Command::ResetZoom, { "Zoom to 100%", "" } },
			{ Command::ZoomIn, { "Zoom in", "" } },
			{ Command::ZoomOut, { "Zoom out", "" } },
			{ Command::ResetRotation, { "Reset rotation", "" } },
			{ Command::RotateLeft, { "Rotate left", "" } },
			{ Command::RotateRight, { "Rotate right", "" } },
			{ Command::FitOnScreen, { "Fit on screen", "" } },
			{ Command::ShowChannel, { "Show channel", "" } },
			{ Command::ShowGrid, { "Show grid", "Show grid" } },
			{ Command::LockGuides, { "Lock guides", "Lock guides" } },
			{ Command::DeleteAllGuides, { "Delete all guides", "Delete all guides" } },
			{ Command::FullScreen, { "Full screen", "" } },
			{ Command::SelectAllAsRectangle, { "Select all as rectangle", "Select all as rectangle" } },
			{ Command::SelectAllAsLasso, { "Select all as lasso", "Select all as lasso" } },
			{ Command::SelectCanvasFrame, { "Select canvas frame", "Select all canvas" } },
			{ Command::Deselect, { "Deselect", "Deselect" } },
			{ Command::InvertSelection, { "Invert selection", "Invert selection" } },
			{ Command::CanvasBackground, { "Canvas background", "" } },
			{ Command::CanvasAppearance, { "Canvas appearance", "" } },
			{ Command::CanvasUnitAndResolution, { "Canvas unit & resolution", "" } },
			{ Command::InfiniteCanvas, { "Infinite canvas", "Infinite canvas" } },
			{ Command::ClearCanvas, { "Clear canvas", "Clear canvas" } },
			{ Command::NewLayer, { "New layer above current", "Create layer" } },
			{ Command::MoveLayerUp, { "Ascend", "Move layer up/down" } },
			{ Command::MoveLayerDown, { "Descend", "Move layer up/down" } },
			{ Command::RenameLayer, { "Rename", "Rename layer" } },
			{ Command::SelectLayerAbove, { "Select layer above current", "Select layer" } },
			{ Command::SelectLayerBelow, { "Select layer below current", "Select layer" } },
			{ Command::SelectAllLayers, { "Select all layers", "Select layer" } },
			{ Command::DuplicateLayer, { "Duplicate", "Duplicate layer" } },
			{ Command::DeleteLayer, { "Delete", "Delete layer" } },
			{ Command::ClearLayer, { "Clear", "Clear layer" } },
			{ Command::MergeLayers, { "Merge", "Merge layers" } },
			{ Command::MergeAllLayers, { "Merge all layers", "Merge all layers" } },
			{ Command::ToggleLayerVisible, { "Visible", "Show/hide layer" } },
			{ Command::ToggleLayerClip, { "Clipping mask", "Lock/unlock layer" } },
			{ Command::ToggleLayerLock, { "Locked", "Lock/unlock layer" } },
			{ Command::ToggleLayerAlphaLock, { "Locked alpha", "Lock/unlock layer" } },
			{ Command::BakeLayerOpacity, { "Bake opacity", "Bake layer opacity" } },
			{ Command::PreviousLayerBlend, { "Previous blend mode", "Layer blend mode" } },
			{ Command::NextLayerBlend, { "Next blend mode", "Layer blend mode" } },
			{ Command::NormalLayerBlend, { "Normal", "Layer blend mode" } },
			{ Command::LayerOpacityDecrease, { "Opacity decrease", "Layer opacity" } },
			{ Command::LayerOpacityIncrease, { "Opacity increase", "Layer opacity" } },
			{ Command::LayerOpacity0, { "Opacity 0%", "Layer opacity" } },
			{ Command::LayerOpacity10, { "Opacity 10%", "Layer opacity" } },
			{ Command::LayerOpacity20, { "Opacity 20%", "Layer opacity" } },
			{ Command::LayerOpacity30, { "Opacity 30%", "Layer opacity" } },
			{ Command::LayerOpacity40, { "Opacity 40%", "Layer opacity" } },
			{ Command::LayerOpacity50, { "Opacity 50%", "Layer opacity" } },
			{ Command::LayerOpacity60, { "Opacity 60%", "Layer opacity" } },
			{ Command::LayerOpacity70, { "Opacity 70%", "Layer opacity" } },
			{ Command::LayerOpacity80, { "Opacity 80%", "Layer opacity" } },
			{ Command::LayerOpacity90, { "Opacity 90%", "Layer opacity" } },
			{ Command::LayerOpacity100, { "Opacity 100%", "Layer opacity" } },
			{ Command::Undo, { "Undo", "" } },
			{ Command::Redo, { "Redo", "" } },
			{ Command::ClearHistory, { "Clear history", "" } },
			{ Command::SetHistoryLimit, { "Set history limit", "" } },
			{ Command::ShowHidePanels, { "Show/hide panels", "" } },
			{ Command::CloseAllPanels, { "Close all panels", "" } },
			{ Command::ToggleConsole, { "Show Console", "" } },
			{ Command::ToggleConsoleFaint, { "Show Console", "" } },
			{ Command::ToggleConsoleOpaque, { "Show Console", "" } },
			{ Command::ShowToolPanel, { "Show Tool", "" } },
			{ Command::ShowLinePanel, { "Show Line", "" } },
			{ Command::ShowBrushListPanel, { "Show Brush list", "" } },
			{ Command::ShowBrushOptionsPanel, { "Show Brush options", "" } },
			{ Command::ShowColorPanel, { "Show Color", "" } },
			{ Command::ShowLayersPanel, { "Show Layers", "" } },
			{ Command::ShowRulers, { "Show rulers", "" } },
			{ Command::ResetAllBrushes, { "Reset all brushes", "" } },
			{ Command::ToolPaint, { "Paint tool", "" } },
			{ Command::ToolPixel, { "Pixel tool", "" } },
			{ Command::ToolEraser, { "Eraser tool", "" } },
			{ Command::ToolSmudge, { "Smudge tool", "" } },
			{ Command::ToolBucket, { "Paint bucket tool", "" } },
			{ Command::ToolPicker, { "Color picker tool", "" } },
			{ Command::ToolRuler, { "Ruler tool", "" } },
			{ Command::ToolMove, { "Move tool", "" } },
			{ Command::ToolMagicWand, { "Magic wand tool", "" } },
			{ Command::ToolLasso, { "Lasso selection tool", "" } },
			{ Command::ToolRectangleSelect, { "Rectangle selection tool", "" } },
			{ Command::ToolGradient, { "Gradient tool", "" } },
			{ Command::ToolSlice, { "Slice tool", "" } },
			{ Command::ToolZoom, { "Zoom tool", "" } },
			{ Command::ToolRotate, { "Rotate tool", "" } },
			{ Command::ToolHand, { "Hand tool", "" } },
			{ Command::ToolExport, { "Export tool", "" } },
			{ Command::ToolExpose, { "Expose tool", "" } },
			{ Command::ToolBackground, { "Background tool", "" } },
			{ Command::CanvasFrameSize, { "Canvas frame size", "" } },
			{ Command::AdjustValue, { "Brightness & exposure", "Adjust value" } },
			{ Command::AdjustColorBalance, { "Color balance", "Color balance" } },
			{ Command::AdjustHueSaturation, { "Hue & saturation", "Hue saturation" } },
			{ Command::AdjustColorize, { "Colorize", "Colorize" } },
			{ Command::AdjustAlpha, { "Alpha", "Alpha" } },
			{ Command::FreeTransform, { "Free Transform", "" } },
			{ Command::HoldHand, { "Hand tool", "" } },
			{ Command::HoldZoom, { "Zoom tool", "" } },
			{ Command::HoldBrushSize, { "Brush size tool", "" } },
			{ Command::HoldBrushAngle, { "Brush angle tool", "" } },
			{ Command::HoldBrushFlow, { "Brush flow tool", "" } },
			{ Command::HoldBrushOpacity, { "Brush opacity tool", "" } },
			{ Command::ToolBrushAngle, { "Brush angle", "" } },
			{ Command::ToolBrushSize, { "Brush size", "" } },
			{ Command::SetBrush, { "Set brush", "" } },
			{ Command::ResetBrush, { "Reset brush", "" } },
			{ Command::BrushFlowDecrease, { "Flow decrease", "" } },
			{ Command::BrushFlowIncrease, { "Flow increase", "" } },
			{ Command::BrushOpacityDecrease, { "Opacity decrease", "" } },
			{ Command::BrushOpacityIncrease, { "Opacity increase", "" } },
			{ Command::BrushAngleDecrease, { "Angle decrease", "" } },
			{ Command::BrushAngleIncrease, { "Angle increase", "" } },
			{ Command::LineNormal, { "Line", "" } },
			{ Command::LineSmooth, { "Smooth", "" } },
			{ Command::LineRuler, { "Ruler", "" } },
			{ Command::LineEllipse, { "Ellipse", "" } },
			{ Command::LinePerspective, { "Perspective", "" } },
			{ Command::LineSymmetry, { "Symmetry", "" } },
			{ Command::LineIsometric, { "Isometric", "" } },
			{ Command::SmallerBrush, { "Smaller brush", "" } },
			{ Command::BiggerBrush, { "Bigger brush", "" } },
			{ Command::StabilizerDecrease, { "Stabilizer decrease", "" } },
			{ Command::StabilizerIncrease, { "Stabilizer increase", "" } },
			{ Command::RecordSession, { "Record session", "" } },
			{ Command::AddSwatch, { "Add color to swatches", "" } },
			{ Command::ClearColorHistory, { "Clear color history", "" } },
			{ Command::SpreadSelection, { "Spread selection", "Spread selection" } },
			{ Command::BlurSelection, { "Blur selection", "Blur selection" } },
			{ Command::FillSelection, { "Fill color", "Fill" } },
			{ Command::FillUnderneath, { "Fill to new layer underneath", "Fill" } },
			{ Command::CopyToNewLayer, { "Copy to new layer", "Copy to new layer" } },
			{ Command::ShowSymmetryLine, { "Show symmetry line", "" } },
			{ Command::MoveSymmetryLine, { "Move symmetry line", "Move symmetry line" } },
			{ Command::LockSymmetryLine, { "Lock symmetry line", "Lock symmetry line" } },
			{ Command::ResetSymmetryLine, { "Reset symmetry line", "Reset symmetry line" } },
			{ Command::MovePerspectivePoint, { "Move perspective point", "Move perspective point" } },
			{ Command::LockPerspective, { "Lock perspective", "Lock perspective" } },
			{ Command::ResetPerspective, { "Reset vanishing points", "Create perspective" } },
			{ Command::StopSessionRecording, { "Stop recording and save", "" } },
			{ Command::BrushOpacity10, { "Brush opacity 10%", "" } },
			{ Command::BrushOpacity20, { "Brush opacity 20%", "" } },
			{ Command::BrushOpacity30, { "Brush opacity 30%", "" } },
			{ Command::BrushOpacity40, { "Brush opacity 40%", "" } },
			{ Command::BrushOpacity50, { "Brush opacity 50%", "" } },
			{ Command::BrushOpacity60, { "Brush opacity 60%", "" } },
			{ Command::BrushOpacity70, { "Brush opacity 70%", "" } },
			{ Command::BrushOpacity80, { "Brush opacity 80%", "" } },
			{ Command::BrushOpacity90, { "Brush opacity 90%", "" } },
			{ Command::BrushOpacity100, { "Brush opacity 100%", "" } },
			{ Command::DefaultColors, { "Default colors", "" } },
			{ Command::SwapColors, { "Swap colors", "" } },
			{ Command::CopyColor, { "Copy", "" } },
			{ Command::PasteColor, { "Paste", "" } },
			{ Command::Cooler, { "Cooler", "" } },
			{ Command::Warmer, { "Warmer", "" } },
			{ Command::HueComplement, { "Hue compliment", "" } },
			{ Command::HueRotateLeft, { "Hue rotate left", "" } },
			{ Command::HueRotateRight, { "Hue rotate right", "" } },
			{ Command::Grayer, { "Grayer", "" } },
			{ Command::Purer, { "Purer", "" } },
			{ Command::Darker, { "Darker", "" } },
			{ Command::Brighter, { "Brighter", "" } },
			{ Command::Lightness0, { "Lightness 0%", "" } },
			{ Command::Lightness10, { "Lightness 10%", "" } },
			{ Command::Lightness20, { "Lightness 20%", "" } },
			{ Command::Lightness30, { "Lightness 30%", "" } },
			{ Command::Lightness40, { "Lightness 40%", "" } },
			{ Command::Lightness50, { "Lightness 50%", "" } },
			{ Command::Lightness60, { "Lightness 60%", "" } },
			{ Command::Lightness70, { "Lightness 70%", "" } },
			{ Command::Lightness80, { "Lightness 80%", "" } },
			{ Command::Lightness90, { "Lightness 90%", "" } },
			{ Command::Lightness100, { "Lightness 100%", "" } },
		};
		static const CommandInfo unknown;

		const auto found = infos.find(command);
		return found == infos.end() ? unknown : found->second;
	}

	void bindDefaultShortcuts(ShortcutMap& shortcuts) {
		shortcuts.bind(ctrl(InputID::N), Command::NewInfiniteCanvas);
		shortcuts.bind(ctrl(InputID::O), Command::Open);
		shortcuts.bind(ctrl(InputID::S), Command::Save);
		shortcuts.bind(ctrlShift(InputID::S), Command::SaveAs);
		shortcuts.bind(ctrl(InputID::X), Command::Cut);
		shortcuts.bind(ctrl(InputID::C), Command::Copy);
		shortcuts.bind(ctrl(InputID::V), Command::Paste);
		shortcuts.bind(ctrl(InputID::F), Command::FlipHorizontal);
		shortcuts.bind(ctrlShift(InputID::F), Command::FlipVertical);
		shortcuts.bind(plain(InputID::INSERT), Command::Transform);
		shortcuts.bind(plain(InputID::DELETE_KEY), Command::Clear);
		shortcuts.bind(ctrl(InputID::DELETE_KEY), Command::Crop);
		shortcuts.bind(plain(InputID::F), Command::FlipViewHorizontal);
		shortcuts.bind(shift(InputID::F), Command::FlipViewVertical);
		shortcuts.bind(ctrl(InputID::G), Command::ShowGrid);
		shortcuts.bind(ctrl(InputID::R), Command::ShowRulers);
		shortcuts.bind(ctrl(InputID::P), Command::ShowChannel);
		shortcuts.bind(ctrl(InputID::MINUS), Command::ZoomOut);
		shortcuts.bind(ctrl(InputID::EQUAL), Command::ZoomIn);
		shortcuts.bind(alt(InputID::MINUS), Command::RotateLeft);
		shortcuts.bind(alt(InputID::EQUAL), Command::RotateRight);
		shortcuts.bind(ctrlShift(InputID::R), Command::ResetRotation);
		shortcuts.bind(ctrl(InputID::KP_SUBTRACT), Command::ZoomOut);
		shortcuts.bind(ctrl(InputID::KP_ADD), Command::ZoomIn);
		shortcuts.bind(ctrl(InputID::N0), Command::FitOnScreen);
		shortcuts.bind(ctrl(InputID::KP_0), Command::FitOnScreen);
		shortcuts.bind(ctrlShift(InputID::N0), Command::ResetZoom);
		shortcuts.bind(ctrlShift(InputID::KP_0), Command::ResetZoom);
		shortcuts.bind(plain(InputID::F11), Command::FullScreen);
		shortcuts.bind(ctrlShift(InputID::N), Command::NewLayer);
		shortcuts.bind(ctrl(InputID::E), Command::MergeLayers);
		shortcuts.bind(ctrl(InputID::K), Command::ClearLayer);
		shortcuts.bind(ctrl(InputID::J), Command::DuplicateLayer);
		shortcuts.bind(ctrlShift(InputID::TAB), Command::SelectLayerAbove);
		shortcuts.bind(ctrl(InputID::TAB), Command::SelectLayerBelow);
		shortcuts.bind(ctrlAlt(InputID::A), Command::SelectAllLayers);
		shortcuts.bind(ctrl(InputID::A), Command::SelectAllAsRectangle);
		shortcuts.bind(ctrlShift(InputID::A), Command::SelectAllAsLasso);
		shortcuts.bind(ctrl(InputID::D), Command::Deselect);
		shortcuts.bind(ctrl(InputID::I), Command::InvertSelection);
		shortcuts.bind(ctrl(InputID::Z), Command::Undo, true);
		shortcuts.bind(ctrl(InputID::Y), Command::Undo, true);
		shortcuts.bind(ctrlShift(InputID::Z), Command::Redo, true);
		shortcuts.bind(ctrlShift(InputID::Y), Command::Redo, true);
		shortcuts.bind(ctrl(InputID::GRAVE_ACCENT), Command::ToggleConsole);
		shortcuts.bind(ctrlShift(InputID::GRAVE_ACCENT), Command::ToggleConsoleFaint);
		shortcuts.bind(ctrlAlt(InputID::GRAVE_ACCENT), Command::ToggleConsoleOpaque);
		shortcuts.bind(plain(InputID::F2), Command::ShowToolPanel);
		shortcuts.bind(plain(InputID::F3), Command::ShowLinePanel);
		shortcuts.bind(plain(InputID::F4), Command::ShowBrushListPanel);
		shortcuts.bind(plain(InputID::F5), Command::ShowBrushOptionsPanel);
		shortcuts.bind(plain(InputID::F6), Command::ShowColorPanel);
		shortcuts.bind(plain(InputID::F7), Command::ShowLayersPanel);
		shortcuts.bind(plain(InputID::P), Command::ToolPixel);
		shortcuts.bind(plain(InputID::B), Command::ToolPaint);
		shortcuts.bind(plain(InputID::E), Command::ToolEraser);
		shortcuts.bind(plain(InputID::S), Command::ToolSmudge);
		shortcuts.bind(plain(InputID::U), Command::ToolBucket);
		shortcuts.bind(plain(InputID::I), Command::ToolPicker);
		shortcuts.bind(plain(InputID::R), Command::ToolRuler);
		shortcuts.bind(plain(InputID::V), Command::ToolMove);
		shortcuts.bind(plain(InputID::W), Command::ToolMagicWand);
		shortcuts.bind(plain(InputID::L), Command::ToolLasso);
		shortcuts.bind(plain(InputID::K), Command::ToolRectangleSelect);
		shortcuts.bind(plain(InputID::G), Command::ToolGradient);
		shortcuts.bind(plain(InputID::O), Command::ToolRotate);
		shortcuts.bind(plain(InputID::H), Command::ToolHand);
		shortcuts.bind(plain(InputID::X), Command::ToolExpose);
		shortcuts.bind(ctrl(InputID::T), Command::FreeTransform);
		shortcuts.bind(ctrl(InputID::L), Command::AdjustValue);
		shortcuts.bind(ctrl(InputID::B), Command::AdjustColorBalance);
		shortcuts.bind(ctrl(InputID::U), Command::AdjustHueSaturation);
		shortcuts.bind(plain(InputID::SPACE), Command::HoldHand);
		shortcuts.bind(alt(InputID::SPACE), Command::HoldHand);
		shortcuts.bind(ctrl(InputID::SPACE), Command::HoldHand);
		shortcuts.bind(shift(InputID::SPACE), Command::HoldHand);
		shortcuts.bind(plain(InputID::Z), Command::HoldZoom);
		shortcuts.bind(plain(InputID::D), Command::HoldBrushSize);
		shortcuts.bind(plain(InputID::A), Command::HoldBrushAngle);
		shortcuts.bind(ctrl(InputID::N1), Command::LineNormal);
		shortcuts.bind(ctrl(InputID::N2), Command::LineSmooth);
		shortcuts.bind(ctrl(InputID::N3), Command::LineRuler);
		shortcuts.bind(ctrl(InputID::N4), Command::LineEllipse);
		shortcuts.bind(ctrl(InputID::N5), Command::LinePerspective);
		shortcuts.bind(ctrl(InputID::N6), Command::LineSymmetry);
		shortcuts.bind(ctrl(InputID::N7), Command::LineIsometric);
		shortcuts.bind(plain(InputID::LEFT_BRACKET), Command::SmallerBrush, true);
		shortcuts.bind(plain(InputID::RIGHT_BRACKET), Command::BiggerBrush, true);
		shortcuts.bind(plain(InputID::N1), Command::BrushOpacity10);
		shortcuts.bind(plain(InputID::N2), Command::BrushOpacity20);
		shortcuts.bind(plain(InputID::N3), Command::BrushOpacity30);
		shortcuts.bind(plain(InputID::N4), Command::BrushOpacity40);
		shortcuts.bind(plain(InputID::N5), Command::BrushOpacity50);
		shortcuts.bind(plain(InputID::N6), Command::BrushOpacity60);
		shortcuts.bind(plain(InputID::N7), Command::BrushOpacity70);
		shortcuts.bind(plain(InputID::N8), Command::BrushOpacity80);
		shortcuts.bind(plain(InputID::N9), Command::BrushOpacity90);
		shortcuts.bind(plain(InputID::N0), Command::BrushOpacity100);
		shortcuts.bind(alt(InputID::Q), Command::HueRotateLeft);
		shortcuts.bind(alt(InputID::W), Command::HueRotateRight);
		shortcuts.bind(alt(InputID::A), Command::Grayer);
		shortcuts.bind(alt(InputID::S), Command::Purer);
		shortcuts.bind(alt(InputID::Z), Command::Darker);
		shortcuts.bind(alt(InputID::X), Command::Brighter);
		shortcuts.bind(alt(InputID::GRAVE_ACCENT), Command::Lightness0);
		shortcuts.bind(alt(InputID::N1), Command::Lightness10);
		shortcuts.bind(alt(InputID::N2), Command::Lightness20);
		shortcuts.bind(alt(InputID::N3), Command::Lightness30);
		shortcuts.bind(alt(InputID::N4), Command::Lightness40);
		shortcuts.bind(alt(InputID::N5), Command::Lightness50);
		shortcuts.bind(alt(InputID::N6), Command::Lightness60);
		shortcuts.bind(alt(InputID::N7), Command::Lightness70);
		shortcuts.bind(alt(InputID::N8), Command::Lightness80);
		shortcuts.bind(alt(InputID::N9), Command::Lightness90);
		shortcuts.bind(alt(InputID::N0), Command::Lightness100);
	}

	CommandRegistry::CommandRegistry() { bindDefaultShortcuts(mShortcuts); }

	void CommandRegistry::define(Command command, Action action, Predicate enabled, Predicate checked) {
		mEntries[command] = { std::move(action), std::move(enabled), std::move(checked) };
	}

	bool CommandRegistry::isDefined(Command command) const { return mEntries.count(command) > 0; }

	bool CommandRegistry::isEnabled(Command command) const {
		const auto found = mEntries.find(command);
		if (found == mEntries.end()) return false;
		return !found->second.enabled || found->second.enabled();
	}

	bool CommandRegistry::isChecked(Command command) const {
		const auto found = mEntries.find(command);
		return found != mEntries.end() && found->second.checked && found->second.checked();
	}

	bool CommandRegistry::isRepeatOf(Command command) const { return mRunningRepeat && mLastCommand == command; }

	bool CommandRegistry::run(Command command, const Event* trigger) {
		if (!isEnabled(command)) return false;

		mRunningRepeat = command == mLastCommand && mClockMs - mLastRunMs < kRepeatWindowMs;
		mEntries.at(command).action(trigger);
		mLastCommand = command;
		mLastRunMs = mClockMs;
		mRunningRepeat = false;
		if (mOnCommandRun) mOnCommandRun(command);
		return true;
	}

	bool CommandRegistry::handleKey(Event& event) {
		const ShortcutMap::Binding* binding = mShortcuts.find(event);
		if (!binding || !isDefined(binding->command)) return false;

		event.accept();
		run(binding->command, &event);
		return true;
	}
}
