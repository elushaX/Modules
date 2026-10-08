#include "ChannelView.hpp"

namespace tp::sketch2d {

	namespace {
		ChannelMatrix greyFrom(halnf r, halnf g, halnf b) { return { r, g, b, 0, r, g, b, 0, r, g, b, 0, 0, 0, 0, 1 }; }
	}

	const char* viewChannelName(ViewChannel channel) {
		switch (channel) {
			case ViewChannel::Red: return "Red";
			case ViewChannel::Green: return "Green";
			case ViewChannel::Blue: return "Blue";
			case ViewChannel::AlphaOnBlack: return "Alpha";
			case ViewChannel::AlphaOnWhite: return "Alpha on white";
			case ViewChannel::Lightness: return "Lightness";
		}
		return "";
	}

	ChannelMatrix channelMatrix(ViewChannel channel) {
		switch (channel) {
			case ViewChannel::Red: return greyFrom(1, 0, 0);
			case ViewChannel::Green: return greyFrom(0, 1, 0);
			case ViewChannel::Blue: return greyFrom(0, 0, 1);
			case ViewChannel::AlphaOnBlack: return { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 };
			case ViewChannel::AlphaOnWhite: return { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
			case ViewChannel::Lightness: return greyFrom(kLumaRed, kLumaGreen, kLumaBlue);
		}
		return greyFrom(1, 0, 0);
	}

	RGBA applyChannelView(ViewChannel channel, const RGBA& premultiplied) {
		const ChannelMatrix matrix = channelMatrix(channel);
		const halnf in[4] = { premultiplied.r, premultiplied.g, premultiplied.b, premultiplied.a };
		halnf out[4] = {};
		for (size_t row = 0; row < 4; row++) {
			for (size_t column = 0; column < 4; column++)
				out[row] += matrix[row * 4 + column] * in[column];
		}
		return { out[0], out[1], out[2], out[3] };
	}

	RGBA channelBackground(ViewChannel channel, const RGBA& background) {
		if (channel == ViewChannel::AlphaOnBlack) return { 0, 0, 0, 1 };
		if (channel == ViewChannel::AlphaOnWhite) return { 1, 1, 1, 1 };

		const RGBA premultiplied = {
			background.r * background.a, background.g * background.a, background.b * background.a, background.a
		};
		return applyChannelView(channel, premultiplied);
	}
}
