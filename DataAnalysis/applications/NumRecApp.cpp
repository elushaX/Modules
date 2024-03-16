
#include "FCNN.hpp"
#include "LocalConnection.hpp"
// #include "NewPlacement.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

using namespace tp;

void loadImage(Buffer<halnf>& output, const char* name) {
	int x, y, channels_in_file;
	unsigned char* loadedImage = stbi_load(name, &x, &y, &channels_in_file, 4);

	if (!loadedImage) return;

	output.reserve(x * y);

	for (auto i : Range(output.size())) {
		output[i] = loadedImage[i * 4] / 255.f;
	}

	stbi_image_free(loadedImage);
}

void loadNN(FCNN& nn) {
	ArchiverLocalConnection<true> archiver;

	archiver.connection.connect(LocalConnection::Location("NumRec.wb"), LocalConnection::Type(true));

	if (archiver.connection.getConnectionStatus().isOpened()) {
		archiver % nn;
	} else {
		Buffer<halni> layers = { 784, 10 };
		nn.initializeRandom(layers);
	}
}

void executeCmd(const char* imageName) {
	FCNN nn;
	Buffer<halnf> output(10);
	Buffer<halnf> input;

	loadNN(nn);
	loadImage(input, imageName);

	nn.evaluate(input, output);

	printf("Output - ");
	for (auto val : output) {
		printf("%f ", val.data());
	}
	printf("\n\n");
}

int main(int argc, char** argv) {
	const char* imageName = "digit.png";

	if (argc == 2) {
		imageName = argv[1];
	}

	executeCmd(imageName);

	return 0;
}