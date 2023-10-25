#pragma once

#include "Buffer.hpp"
#include "DataAnalysisCommon.hpp"

namespace tp {
	// Fully connected neural network
	class FCNN {

		struct Layer {

			struct Neuron {
				struct Parameter {
					Parameter() = default;

					Parameter(halnf in) :
						val(in) {}

					halnf val = 0;
					halnf grad = 0;
				};

				Parameter bias;
				Buffer<Parameter> weights;

				halnf activationValue = 0;
				halnf activationValueLinear = 0;

				halnf cache;
			};

			Buffer<Neuron> neurons;
		};

	public:
		FCNN() = default;
		explicit FCNN(const Buffer<halni>& description);

		void initializeRandom(const Buffer<halni>& description);
		void evaluate(const Buffer<halnf>& input, Buffer<halnf>& output);

		halnf calcCost(const Buffer<halnf>& output);

		void clearGrad();
		void calcGrad(const Buffer<halnf>& output);
		void applyGrad(halnf step);

	public:
		Buffer<Layer> mLayers;
		halni mAvgCount = 0;
	};
};
