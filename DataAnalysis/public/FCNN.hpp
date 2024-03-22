#pragma once

#include "Buffer.hpp"

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

				public:
					template <class tArchiver>
					void archive(tArchiver& ar) {
						ar % val;
					}
				};

				Parameter bias;
				Buffer<Parameter> weights;

				halnf activationValue = 0;
				halnf activationValueLinear = 0;

				halnf cache;

			public:
				template <class tArchiver>
				void archive(tArchiver& ar) {
					ar % bias;
					ar % weights;
				}
			};

			Buffer<Neuron> neurons;

		public:
			template <class tArchiver>
			void archive(tArchiver& ar) {
				ar % neurons;
			}
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
		template <class tArchiver>
		void archive(tArchiver& ar) {
			ar % mLayers;
		}

	public:
		Buffer<Layer> mLayers;
		halni mAvgCount = 0;
	};
};
