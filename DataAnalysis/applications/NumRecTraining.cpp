#include "FCNN.hpp"
#include "LocalConnection.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace tp;

void writeImage(const Buffer<halnf>& image, const char* name) {
	struct Tmp {
		uint1 r, g, b, a;
	};

	Buffer<Tmp> converted;
	converted.reserve(image.size());

	for (auto i = 0; i < image.size(); i++) {
		auto val = uint1(image[i] * 255);
		converted[i] = { val, val, val, 255 };
	}

	stbi_write_png(name, 28, 28, 4, converted.getBuff(), 28 * 4);
}

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

struct NumberRec {
	NumberRec() {

		// try to load wb file
		{
			ArchiverLocalConnection<true> archiver;

			archiver.connection.connect(LocalConnection::Location("NumRec.wb"), LocalConnection::Type(true));

			if (archiver.connection.getConnectionStatus().isOpened()) {
				archiver % nn;
			} else {
				Buffer<halni> layers = { 784, 10 };
				nn.initializeRandom(layers);
			}
		}

		Dataset dataset;

		if (!loadDataset(dataset, "rsc/mnist")) {
			printf("Cant Load Mnist Dataset\n");
			return;
		}

		mTestcases.reserve(dataset.images.size());
		for (auto i : Range(dataset.images.size())) {
			auto& image = dataset.images[i];
			auto label = dataset.labels[i];

			auto& testcase = mTestcases[i];

			testcase.output.reserve(10);

			for (auto dig : Range(10)) {
				testcase.output[dig] = label == dig ? 1 : 0;
			}

			testcase.input.reserve(image.size());

			for (auto pxl : Range(image.size())) {
				testcase.input[pxl] = (halnf) image[pxl] / 255.f;
			}
		}

		output.reserve(10);

		writeImage(mTestcases.first().input, "tmp1.png");
		writeImage(mTestcases.last().input, "tmp2.png");
	}

	~NumberRec() {
		// save aas wb file
		{
			ArchiverLocalConnection<false> archiver;
			archiver.connection.connect(LocalConnection::Location("NumRec.wb"), LocalConnection::Type(false));
			if (archiver.connection.getConnectionStatus().isOpened()) {
				archiver % nn;
			}
		}
	}

	halnf eval(ualni idx) {
		nn.evaluate(mTestcases[idx].input, output);
		return nn.calcCost(mTestcases[idx].output);
	}

	void applyGrad(ualni idx) {
		nn.calcGrad(mTestcases[idx].output);
		nn.applyGrad(step);
	}

	static halni getMaxIdx(const Buffer<halnf>& in) {
		halni out = 0;
		for (auto i : Range(in.size())) {
			if (in[i] > in[out]) {
				out = i;
			}
		}
		return out;
	}

	bool testIncorrect(ualni idx) {
		nn.evaluate(mTestcases[idx].input, output);
		return getMaxIdx(mTestcases[idx].output) != getMaxIdx(output);
	}

	void debLog(halni idx) {
		printf("\n Got      %i - ", getMaxIdx(output));
		for (auto val : output) {
			printf("%f ", val.data());
		}
		printf("\n Expected %i - ", getMaxIdx(mTestcases[idx].output));
		for (auto val : mTestcases[idx].output) {
			printf("%f ", val.data());
		}
		printf("\n\n");
	}

	void displayImage(ualni idx) {
		auto& testcase = mTestcases[idx];
		printf("Image : %i\n", int(getMaxIdx(testcase.output)));
		for (auto i : Range(28)) {
			for (auto j : Range(28)) {
				printf("%c", char(testcase.input[j * 28 + i] * 255));
			}
			printf("\n");
		}
	}

	halnf test(const Range<halni>& range) {
		halnf avgCost = 0;
		for (auto i : range) {
			avgCost += eval(i);
		}
		avgCost /= (halnf) range.idxDiff();
		return avgCost;
	}

	void trainStep(const Range<halni>& range) {
		nn.clearGrad();
		for (auto i : range) {
			nn.evaluate(mTestcases[i].input, output);
			nn.calcGrad(mTestcases[i].output);
		}
		nn.applyGrad(step);
	}

public:
	struct Image {
		Buffer<halnf> input;
		Buffer<halnf> output;
	};

public:
	Buffer<Image> mTestcases;

	FCNN nn;
	Buffer<halnf> output;

	halnf step = 1.f;
};

int main() {
	ModuleManifest* deps[] = { &gModuleDataAnalysis, &gModuleConnection, nullptr };
	ModuleManifest module = ModuleManifest("NumRec", nullptr, nullptr, deps);

	if (!module.initialize()) {
		return 1;
	}

	{
		NumberRec app;

		auto numBatches = 10;
		auto trainRange = Range(0, 50000);
		auto testRange = Range(50000, 70000);

		auto batchSize = trainRange.idxDiff() / numBatches;

		for (auto epoch : Range(1)) {
			printf("Epoch %i\n", epoch.index());

			for (auto batchIdx : Range(trainRange.idxDiff() / batchSize)) {
				printf(" - Batch :%i \n", batchIdx.index());

				auto batchRange = Range(trainRange.idxBegin() + batchSize * batchIdx, trainRange.idxBegin() + batchSize * (batchIdx + 1));

				app.trainStep(batchRange);

				printf("Cost on batch data : %f\n", app.test(batchRange));
			}

			printf("Cost on test data : %f\n\n", app.test(testRange));
		}

		auto errors = 0;
		for (auto i : testRange) {
			if (app.testIncorrect(i)) {
				errors++;
			}
			// app.debLog(i);
			// app.displayImage(i);
		}

		printf("\n\nIncorrect - %i out of %i (%f)\n\n", errors, testRange.idxDiff(), (halnf) errors / (halnf) testRange.idxDiff());
	}

	module.deinitialize();

	return 0;
}