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
				halnf mActivationValueLinear = 0;
			};

			Buffer<Neuron> mNeurons;
		};

		struct LayerCache {
			struct NeuronCache {
				halnf mBiasGrad = 0;
				Buffer<halnf> mWeightsGrad;
			};

			halnf mCache = 0;
			Buffer<NeuronCache> mNeurons;
		};

	public:
		FullyConnectedNN() = default;

		void initializeRandom(Buffer<halni> description);
		void evaluate(const Buffer<halnf>& input, Buffer<halnf>& output);

		void clearGrad();
		halnf calcCost(const Buffer<halnf>& output);
		void calcGrad(const Buffer<halnf>& output);
		void applyGrad();

	private:
		Buffer<Layer> mLayers;
		Buffer<LayerCache> mLayersCache;
		halni mAvgCount = 0;
	};
};
