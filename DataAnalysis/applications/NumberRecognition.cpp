#include "FullyConnectedNN.hpp"
#include "LocalConnection.hpp"
#include "NewPlacement.hpp"

using namespace tp;

struct Dataset {
	ualni length = 0;
	Pair<ualni, ualni> imageSize = { 0, 0 };
	Buffer<uint1> labels;
	Buffer<Buffer<uint1>> images;
};

bool loadDataset(Dataset& out, const String& location) {
	LocalConnection dataset;
	dataset.connect(LocalConnection::Location(location), LocalConnection::Type(true));

	if (!dataset.getConnectionStatus().isOpened()) {
		return false;
	}

	LocalConnection::Byte length;
	dataset.readBytes(&length, 1);

	LocalConnection::Byte sizeX;
	dataset.readBytes(&sizeX, 1);

	LocalConnection::Byte sizeY;
	dataset.readBytes(&sizeY, 1);

	out.length = ((ualni) length) * 1000;
	out.imageSize = { sizeX, sizeY };

	out.labels.reserve(out.length);
	out.images.reserve(out.length);

	for (auto i : Range(out.length)) {
		auto& image = out.images[i];
		image.reserve(sizeX * sizeY);
		dataset.readBytes((LocalConnection::Byte*) image.getBuff(), sizeX * sizeY);
	}

	LocalConnection::Byte label;
	dataset.readBytes((LocalConnection::Byte*) out.labels.getBuff(), out.length);

	return true;
}

halnf test(const Dataset& dataset, FullyConnectedNN& nn, Range<ualni> range) {
	ualni numFailed = 0;

	for (auto i : range) {
		auto& image = dataset.images[i];
		auto label = dataset.labels[i];

		Buffer<halnf> results;
		Buffer<halnf> input;

		results.reserve(10);
		input.reserve(image.size());

		for (auto pixelIdx : Range(image.size())) {
			input[pixelIdx] = (halnf) image[pixelIdx] / 255.f;
		}

		nn.evaluate(input, results);

		ualni resultNumber = 0;
		for (auto resIdx : Range(results.size())) {
			if (results[resIdx] > results[resultNumber]) {
				resultNumber = resIdx;
			}
		}

		if (resultNumber != label) {
			numFailed++;
		}
	}

	return (halnf) numFailed / (halnf) range.idxDiff();
}

void testTraining(const Dataset& dataset, FullyConnectedNN& nn) {

	auto propagate = [&](ualni idx) {
		auto& image = dataset.images[idx];
		auto label = dataset.labels[idx];

		Buffer<halnf> results;
		Buffer<halnf> input;

		results.reserve(10);
		input.reserve(image.size());

		for (auto pixelIdx : Range(image.size())) {
			input[pixelIdx] = (halnf) image[pixelIdx] / 255.f;
		}

		nn.evaluate(input, results);

		ualni resultNumber = 0;
		for (auto resIdx : Range(results.size())) {
			if (results[resIdx] > results[resultNumber]) {
				resultNumber = resIdx;
			}
		}

		Buffer<halnf> resultsExpected;
		resultsExpected.reserve(10);
		for (auto resIdx : Range(results.size())) {
			resultsExpected[resIdx] = (resIdx == label) ? 1 : 0;
		}

		nn.calcGrad(resultsExpected);

		return resultNumber;
	};

	nn.clearGrad();

	propagate(0);
	nn.applyGrad();

	propagate(0);
	nn.applyGrad();

	propagate(0);
	nn.applyGrad();

	auto errorPercentage = test(dataset, nn, { 0, 1 });
	printf("Percentage error Trained on first image: %f\n", errorPercentage);
}

void numRec() {
	Dataset dataset;
	FullyConnectedNN nn;

	// settings
	Buffer<halni> layers;
	layers = { 784, 128, 10 };
	halnf trainBatchPercentage = 0.1f;
	halnf testSizePercentage = 0.1f;

	if (!loadDataset(dataset, "rsc/mnist")) {
		printf("Cant Load Mnist Dataset\n");
		return;
	}

	nn.initializeRandom(layers);

	auto errorPercentage = test(dataset, nn, { 0, 100 });

	printf("Percentage error : %f\n", errorPercentage);

	testTraining(dataset, nn);
}

int main() {
	ModuleManifest* deps[] = { &gModuleDataAnalysis, &gModuleConnection, nullptr };
	ModuleManifest module = ModuleManifest("NumRec", nullptr, nullptr, deps);

	if (!module.initialize()) {
		return 1;
	}

	numRec();

	module.deinitialize();

	return 0;
}
