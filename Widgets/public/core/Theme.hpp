#pragma once

#include "Color.hpp"
#include "Geometry.hpp"

namespace tp::ui {

	struct Palette {
		RGBA window = { 0.118f, 0.122f, 0.133f, 1.0f };
		RGBA surface = { 0.149f, 0.153f, 0.165f, 1.0f };
		RGBA raisedSurface = { 0.180f, 0.188f, 0.204f, 1.0f };
		RGBA border = { 0.224f, 0.231f, 0.247f, 1.0f };
		RGBA focusRing = { 0.212f, 0.420f, 0.792f, 0.55f };

		RGBA control = { 0.200f, 0.208f, 0.224f, 1.0f };
		RGBA controlHovered = { 0.243f, 0.251f, 0.271f, 1.0f };
		RGBA controlPressed = { 0.282f, 0.290f, 0.314f, 1.0f };
		RGBA controlBorder = { 0.294f, 0.302f, 0.325f, 1.0f };
		RGBA field = { 0.157f, 0.161f, 0.176f, 1.0f };
		RGBA track = { 0.247f, 0.255f, 0.275f, 1.0f };

		RGBA accent = { 0.212f, 0.420f, 0.792f, 1.0f };
		RGBA accentHovered = { 0.243f, 0.459f, 0.839f, 1.0f };
		RGBA accentPressed = { 0.184f, 0.373f, 0.718f, 1.0f };
		RGBA onAccent = { 1.0f, 1.0f, 1.0f, 1.0f };
		RGBA selection = { 0.180f, 0.263f, 0.431f, 1.0f };
		RGBA neutralSelection = { 0.255f, 0.263f, 0.282f, 1.0f };
		RGBA menuHighlight = { 0.290f, 0.298f, 0.322f, 1.0f };
		RGBA sliderFill = { 0.620f, 0.631f, 0.659f, 1.0f };
		RGBA toggleOn = { 0.878f, 0.886f, 0.902f, 1.0f };
		RGBA toggleOnHovered = { 0.949f, 0.953f, 0.961f, 1.0f };
		RGBA onToggle = { 0.118f, 0.122f, 0.133f, 1.0f };

		RGBA text = { 0.875f, 0.882f, 0.898f, 1.0f };
		RGBA mutedText = { 0.588f, 0.600f, 0.631f, 1.0f };
		RGBA disabledText = { 0.420f, 0.431f, 0.459f, 1.0f };
		RGBA warningText = { 0.937f, 0.690f, 0.290f, 1.0f };
		RGBA separator = { 0.200f, 0.208f, 0.224f, 1.0f };

		RGBA tooltip = { 0.208f, 0.216f, 0.231f, 1.0f };
		RGBA scrim = { 0.0f, 0.0f, 0.0f, 0.35f };
		RGBA shadow = { 0.0f, 0.0f, 0.0f, 0.30f };
		RGBA windowCloseHovered = { 0.769f, 0.263f, 0.247f, 1.0f };
		RGBA windowClosePressed = { 0.659f, 0.212f, 0.200f, 1.0f };
		RGBA onWindowClose = { 1.0f, 1.0f, 1.0f, 1.0f };

		void setAccent(const RGBA& accent);
		static RGBA mix(const RGBA& from, const RGBA& to, halnf amount);
	};

	struct Metrics {
		halnf fontSize = 13;
		halnf smallFontSize = 12;
		halnf lineHeight = 18;
		halnf cornerRadius = 8;
		halnf popupCornerRadius = 11;
		halnf surfaceCornerRadius = 12;
		halnf indicatorCornerRadius = 4;
		halnf surfaceGap = 4;
		halnf borderWidth = 1;

		Insets controlPadding = Insets::symmetric(12, 4);
		halnf controlHeight = 26;
		halnf iconButtonSize = 22;
		halnf windowButtonWidth = 34;
		halnf indicatorSize = 16;
		halnf spacing = 6;

		halnf sliderTrackThickness = 4;
		halnf sliderHandleSize = 14;
		halnf sliderTickLength = 10;
		halnf sliderTickThickness = 1;
		halnf sliderTickSpacing = 4;
		halnf scrollBarThickness = 22;
		halnf scrollTrackThickness = 10;
		halnf scrollTrackEdgeGap = 8;
		halnf splitterThickness = 5;
		halnf separatorThickness = 1;

		halnf menuItemHeight = 24;
		Insets menuPadding = Insets::uniform(5);
		halnf popupGap = 3;

		halnf tooltipDelayMs = 600;
		Insets tooltipPadding = Insets::symmetric(8, 5);

		[[nodiscard]] Metrics scaled(halnf factor) const;
	};

	struct Theme {
		Palette palette;
		Metrics metrics;

		[[nodiscard]] Theme withAccent(const RGBA& accent) const;
		[[nodiscard]] Theme scaled(halnf factor) const;

		static const Theme& standard();
	};

	namespace accents {
		inline const RGBA blue = { 0.212f, 0.420f, 0.792f, 1.0f };
		inline const RGBA purple = { 0.553f, 0.361f, 0.808f, 1.0f };
		inline const RGBA green = { 0.247f, 0.584f, 0.380f, 1.0f };
		inline const RGBA orange = { 0.820f, 0.486f, 0.192f, 1.0f };
		inline const RGBA pink = { 0.800f, 0.322f, 0.553f, 1.0f };
	}
}
