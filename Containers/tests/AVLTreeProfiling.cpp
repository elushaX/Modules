#include <iostream>
#include "Tree.hpp"
#include "Timing.hpp"

using namespace tp;

struct Item {
	Item() :
		data(0) {}
	bool presents = false;
	alni data;
};

const auto size = 1000000;
Item buff[size];

int main() {
	AvlTree<AvlNumericKey<alni>, alni> tree;

	for (auto i : IterRange(size)) {
		buff[i].data = i;
	}

	Timer insertTime;

	ualni loadSize = 0;
	while (loadSize < size / 2) {
		auto idx = ualni(randomFloat() * (size - 1));
		if (!buff[idx].presents) {
			tree.insert((alni) buff[idx].data, buff[idx].data);
			loadSize++;
			buff[idx].presents = true;
		}
	}

	std::cout << "AVL Tree Insert Speed " << (double) loadSize / (double) insertTime.timePassed() << "\n";

	Timer removeTime;

	ualni unloadSize = 0;
	while (unloadSize < size / 2) {
		auto idx = ualni(randomFloat() * (size - 1));
		if (buff[idx].presents) {
			tree.remove((alni) buff[idx].data);
			unloadSize++;
			buff[idx].presents = false;
		}
	}

	for (auto& item : buff) {
		if (item.presents) {
			tree.remove((alni) item.data);
			unloadSize++;
			item.presents = false;
		}
	}

	std::cout << "AVL Tree Remove Speed " << (double) unloadSize / (double) removeTime.timePassed() << "\n";
}