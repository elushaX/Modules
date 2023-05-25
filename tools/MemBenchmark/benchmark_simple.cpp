

#include "allocators.h"

#include <stdio.h>
#include "timer.h"

struct AllocSpeedTets {
	const char* test_desc = NULL;
	virtual void exec() {};
};

struct Test1 : AllocSpeedTets {
	Test1() {
		test_desc = "allocate lots of memory of same sizes, then free it all";
	}
	
	const int len = 1000;
	int size = 100;
	void* buff[1000];

	void exec_util(tp::AbstractAllocator* alloc) {
		for (size_t i = 0; i < len; i++) {
			buff[i] = alloc->Alloc(size);
		}
		for (size_t i = 0; i < len; i++) {
			alloc->Free(buff[i]);
		}
	}

	void exec() {
		tp::time_ms start, end;

		tp::HeapAlloc malloc;
		tp::PoolAlloc pool(0, 0);
		tp::ChunkAlloc chunck(0, 0);
		tp::PickAlloc pick;
		
		start = tp::get_time();
		
		end = tp::get_time();
		printf("Malloc : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pool : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Chunk : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pick : %lli ms \n", end - start);
	}
};

struct Test2 : AllocSpeedTets {
	Test2() {
		test_desc = "allocate lots of memory of different sizes, then free it all";
	}

	void exec() {
		tp::time_ms start, end;

		tp::HeapAlloc malloc;
		tp::PoolAlloc pool(0, 0);
		tp::ChunkAlloc chunck(0, 0);
		tp::PickAlloc pick;

		start = tp::get_time();
		end = tp::get_time();
		printf("Malloc : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pool : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Chunk : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pick : %lli ms \n", end - start);
	}
};

struct Test3 : AllocSpeedTets {
	Test3() {
		test_desc = "allocate only a few blocks of memory, free them, and repeat this loop several times \
		\n (repeat for same - sized blocks and different - sized blocks)\n";
	}

	void exec() {
		tp::time_ms start, end;

		tp::HeapAlloc malloc;
		tp::PoolAlloc pool(0, 0);
		tp::ChunkAlloc chunck(0, 0);
		tp::PickAlloc pick;

		start = tp::get_time();
		end = tp::get_time();
		printf("Malloc : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pool : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Chunk : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pick : %lli ms \n", end - start);
	}
};

struct Test4 : AllocSpeedTets {
	Test4() {
		test_desc = "allocate lots of memory of different sizes, free half of it(e.g.the even allocations), then allocateand free memory in a loop\n";
	}

	void exec() {
		tp::time_ms start, end;

		tp::HeapAlloc malloc;
		tp::PoolAlloc pool(0, 0);
		tp::ChunkAlloc chunck(0, 0);
		tp::PickAlloc pick;

		start = tp::get_time();
		end = tp::get_time();
		printf("Malloc : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pool : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Chunk : %lli ms \n", end - start);

		start = tp::get_time();
		end = tp::get_time();
		printf("Pick : %lli ms \n", end - start);
	}
};

void banckmark() {

	tp::alloc_init();

	const int tests_len = 1;
	AllocSpeedTets* tests[] = {
		new Test1(),
		new Test2(),
		new Test3(),
		new Test4(),
	};

	for (int i = 0; i < tests_len; i++) {
		printf("Test %i: \n	%s\n", i, tests[i]->test_desc);
		tests[i]->exec();

		delete tests[i];
	}

	tp::alloc_uninit();
}