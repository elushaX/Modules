#pragma once

#include "Buffer.hpp"
#include "DataAnalysisCommon.hpp"

namespace tp {
	class FullyConnectedNN {
		struct Layer {
			struct Neuron {
				halnf mBias = 0;
				Buffer<halnf> mWeights;
				halnf mActivationValue = 0;
			};

			Buffer<Neuron> mNeurons;
		};

	public:
		FullyConnectedNN() = default;

		void initializeRandom(Buffer<halni> description);
		void evaluate(const Buffer<halnf>& input, Buffer<halnf>& output);

	private:
		Buffer<Layer> mLayers;
	};
};
