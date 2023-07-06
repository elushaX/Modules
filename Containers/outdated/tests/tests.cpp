
#include "containers.h"

void avl_test();
void map_test();
void queue_test();
void list_test();
void fat_nodes_test();

int main() {
	tp::alloc_init();

	map_test();

	tp::alloc_uninit();
}