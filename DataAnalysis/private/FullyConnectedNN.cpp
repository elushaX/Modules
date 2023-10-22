#include "FullyConnectedNN.hpp"
#include "NewPlacement.hpp"
#include "Utils.hpp"

using namespace tp;

static halnf sigmoid(halnf val) { return 0; }

static halnf relu(halnf val) { return val < 0 ? 0 : val; }

void FullyConnectedNN::initializeRandom(Buffer<halni> description) {
	mLayers.reserve(description.size());

	for (auto i : Range<halni>(0, (halni) description.size())) {
		mLayers[i].mNeurons.reserve(description[i]);
		if (i == 0) {
			continue;
		}
		for (auto neuron : mLayers[i].mNeurons) {
			neuron->mWeights.reserve(description[i - 1]);
			for (auto weight : neuron->mWeights) {
				weight.data() = (halnf) randomFloat();
			}
			neuron->mBias = (halnf) randomFloat();
		}
	}
}

void FullyConnectedNN::evaluate(const Buffer<halnf>& input, Buffer<halnf>& output) {
	ASSERT(output.size() == mLayers.last().mNeurons.size() && input.size() == mLayers.first().mNeurons.size())

	for (auto idx : Range(input.size())) {
		mLayers.first().mNeurons[idx].mActivationValue = input[idx];
	}

	for (auto layerIdx : Range<halni>(1, (halni) mLayers.size())) {
		auto& layer = mLayers[layerIdx];
		auto& layerPrev = mLayers[layerIdx - 1];

		for (auto neuron : layer.mNeurons) {
			neuron->mActivationValue = 0;
			for (auto connectionIdx : Range(neuron->mWeights.size())) {
				neuron->mActivationValue += neuron->mWeights[connectionIdx] * layerPrev.mNeurons[connectionIdx].mActivationValue;
			}
			neuron->mActivationValue += neuron->mBias;
			neuron->mActivationValue = relu(neuron->mActivationValue);
		}
	}

	for (auto idx : Range(output.size())) {
		output[idx] = mLayers.last().mNeurons[idx].mActivationValue;
	}
}
