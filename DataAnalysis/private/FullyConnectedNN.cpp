// #include "NewPlacement.hpp"

#include "FullyConnectedNN.hpp"
#include "Utils.hpp"

#include <cmath>

using namespace tp;

static halnf linearFun(halnf val) { return val; }

static halnf linearFunDerivative(halnf val) { return 1; }

static halnf sigmoid(double val) { return 1.0f / (1.0f + (halnf) exp(-val)); }

static halnf sigmoidDerivative(double val) {
	halnf sigmoid_val = sigmoid(val);
	return sigmoid_val * (1.0f - sigmoid_val);
}

static halnf relu(halnf val) { return val < 0 ? 0 : val; }

static halnf reluDerivative(halnf val) { return val < 0 ? 0 : 1; }

static halnf activationFunction(halnf val) { return sigmoid(val); }

static halnf activationFunctionDerivative(halnf val) { return sigmoidDerivative(val); }

void FullyConnectedNN::initializeRandom(const Buffer<halni>& description) {
	ASSERT(description.size() > 1);

	mLayers.reserve(description.size());

	for (auto i : Range<halni>(0, (halni) description.size())) {
		mLayers[i].neurons.reserve(description[i]);
		if (i == 0) {
			continue;
		}
		for (auto neuron : mLayers[i].neurons) {
			neuron->weights.reserve(description[i - 1]);
			for (auto weight : neuron->weights) {
				weight->val = (halnf) (randomFloat() - 0) * 2;
			}
			neuron->bias.val = (halnf) (randomFloat() - 0) * 2;
		}
	}
}

void FullyConnectedNN::evaluate(const Buffer<halnf>& input, Buffer<halnf>& output) {
	ASSERT(output.size() == mLayers.last().neurons.size() && input.size() == mLayers.first().neurons.size())

	for (auto idx : Range(input.size())) {
		mLayers.first().neurons[idx].activationValue = input[idx];
	}

	for (auto layerIdx : Range<halni>(1, (halni) mLayers.size())) {
		auto& layer = mLayers[layerIdx];
		auto& layerPrev = mLayers[layerIdx - 1];

		for (auto neuron : layer.neurons) {
			neuron->activationValue = 0;
			for (auto connectionIdx : Range(neuron->weights.size())) {
				neuron->activationValue += neuron->weights[connectionIdx].val * layerPrev.neurons[connectionIdx].activationValue;
			}
			neuron->activationValue += neuron->bias.val;
			neuron->activationValueLinear = neuron->activationValue;
			neuron->activationValue = activationFunction(neuron->activationValue);
		}
	}

	for (auto idx : Range(output.size())) {
		output[idx] = mLayers.last().neurons[idx].activationValue;
	}
}

halnf FullyConnectedNN::calcCost(const Buffer<halnf>& output) {
	halnf out = 0;
	for (auto neuronIdx : Range(mLayers.last().neurons.size())) {
		out += pow(output[neuronIdx] - mLayers.last().neurons[neuronIdx].activationValue, 2);
	}
	return out;
}

void FullyConnectedNN::calcGrad(const Buffer<halnf>& output) {
	ASSERT(mLayers.last().neurons.size() == output.size())

	auto& lastLayer = mLayers.last();

	// calculate chaining cache value for each neuron in last layer
	for (auto neuronIdx : Range(lastLayer.neurons.size())) {
		auto& neuron = lastLayer.neurons[neuronIdx];
		neuron.cache = 2 * (neuron.activationValue - output[neuronIdx]);
	}

	// calculate rest of the layers
	for (auto layerIdx = mLayers.size() - 1; layerIdx > 0; layerIdx--) {

		auto& currentLayer = mLayers[layerIdx];
		auto& inputLayer = mLayers[layerIdx - 1];

		for (auto currentNeuronIdx : Range(currentLayer.neurons.size())) {
			auto& currentNeuron = currentLayer.neurons[currentNeuronIdx];

			// calculate cache value (chaining)
			if (layerIdx != mLayers.size() - 1) {
				auto& userLayer = mLayers[layerIdx + 1];

				currentNeuron.cache = 0;
				for (auto userNeuron : userLayer.neurons) {
					currentNeuron.cache += userNeuron->weights[currentNeuronIdx].val * userNeuron->cache;
				}
				currentNeuron.cache *= activationFunctionDerivative(currentNeuron.activationValueLinear);
			}

			// gradient for current neuron bias
			currentNeuron.bias.grad = currentNeuron.cache;

			// calculate gradient for weights of current neuron
			for (auto weightIdx : Range(currentNeuron.weights.size())) {
				currentNeuron.weights[weightIdx].grad = inputLayer.neurons[weightIdx].activationValue * currentNeuron.cache;
			}
		}
	}

	mAvgCount++;
}

void FullyConnectedNN::applyGrad(halnf step) {

	for (auto layIdx : Range<halni>(1, (halni) mLayers.size())) {
		auto& layer = mLayers[layIdx];

		for (auto neuron : layer.neurons) {
			neuron->bias.val -= neuron->bias.grad / (halnf) mAvgCount * step;
			for (auto weightIdx : Range(neuron->weights.size())) {
				neuron->weights[weightIdx].val -= (neuron->weights[weightIdx].grad / (halnf) mAvgCount) * step;
			}
		}
	}

	mAvgCount = 0;
}