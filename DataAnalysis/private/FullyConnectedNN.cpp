#include "FullyConnectedNN.hpp"
#include "NewPlacement.hpp"
#include "Utils.hpp"

using namespace tp;

static halnf sigmoid(halnf val) { return 0; }

static halnf relu(halnf val) { return val < 0 ? 0 : val; }

static halnf reluDerivative(halnf val) { return val < 0 ? 0 : 1; }

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
				weight.data() = (halnf) (randomFloat() - 0.5) * 2;
			}
			neuron->mBias = (halnf) (randomFloat() - 0.5) * 2;
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
			neuron->mActivationValueLinear = neuron->mActivationValue;
			neuron->mActivationValue = relu(neuron->mActivationValue);
		}
	}

	for (auto idx : Range(output.size())) {
		output[idx] = mLayers.last().mNeurons[idx].mActivationValue;
	}
}

halnf FullyConnectedNN::calcCost(const Buffer<halnf>& output) {
	halnf out = 0;
	for (auto neuronIdx : Range(mLayers.last().mNeurons.size())) {
		out += pow(output[neuronIdx] - mLayers.last().mNeurons[neuronIdx].mActivationValue, 2);
	}
	return out;
}

void FullyConnectedNN::clearGrad() {
	mLayersCache.reserve(mLayers.size());
	for (auto layIdx : Range<halni>(0, (halni) mLayers.size())) {
		mLayersCache[layIdx].mNeurons.reserve(mLayers[layIdx].mNeurons.size());
		mLayersCache[layIdx].mCache = 1;

		for (auto weightIdx : Range(mLayersCache[layIdx].mNeurons.size())) {
			mLayersCache[layIdx].mNeurons[weightIdx].mBiasGrad = 0;
			mLayersCache[layIdx].mNeurons[weightIdx].mWeightsGrad.reserve(mLayers[layIdx].mNeurons[weightIdx].mWeights.size());
			for (auto weight : mLayersCache[layIdx].mNeurons[weightIdx].mWeightsGrad) {
				weight.data() = 0;
			}
		}
	}
	mAvgCount = 0;
}

void FullyConnectedNN::calcGrad(const Buffer<halnf>& output) {
	ASSERT(mLayers.last().mNeurons.size() == output.size())

	auto& lastLayer = mLayers.last();
	auto& lastCache = mLayersCache.last();

	lastCache.mCache = 1;

	for (auto neuronIdx : Range(lastLayer.mNeurons.size())) {
		auto& neuronCache = lastCache.mNeurons[neuronIdx];
		auto& neuron = lastLayer.mNeurons[neuronIdx];
		lastCache.mCache += 2 * pow(output[neuronIdx] - neuron.mActivationValue, 2);
	}

	for (auto layerIdx = mLayers.size() - 1; layerIdx > 0; layerIdx--) {
		auto& layer = mLayers[layerIdx];
		auto& cache = mLayersCache[layerIdx];

		auto& layerNext = mLayers[layerIdx - 1];
		auto& cacheNext = mLayersCache[layerIdx - 1];

		cacheNext.mCache = 1;

		for (auto neuronIdx : Range(layer.mNeurons.size())) {
			auto& neuron = layer.mNeurons[neuronIdx];
			auto& neuronCache = cache.mNeurons[neuronIdx];

			auto tmp = cache.mCache * reluDerivative(neuron.mActivationValueLinear);

			neuronCache.mBiasGrad = tmp;

			for (auto weightIdx : Range(neuron.mWeights.size())) {
				neuronCache.mWeightsGrad[weightIdx] = tmp * layerNext.mNeurons[weightIdx].mActivationValue;
			}

			cacheNext.mCache += tmp;
		}
	}
}

void FullyConnectedNN::applyGrad() {
	for (auto layIdx : Range<halni>(1, (halni) mLayers.size())) {
		auto& layerCache = mLayersCache[layIdx];
		auto& layer = mLayers[layIdx];

		for (auto neuronIdx : Range(layer.mNeurons.size())) {
			auto& neuron = layer.mNeurons[neuronIdx];
			auto& neuronCache = layerCache.mNeurons[neuronIdx];

			neuron.mBias += neuronCache.mBiasGrad / (halnf) mAvgCount;

			for (auto weightIdx : Range(neuron.mWeights.size())) {
				neuron.mWeights[weightIdx] += neuronCache.mWeightsGrad[weightIdx] / (halnf) mAvgCount;
			}
		}
	}
}