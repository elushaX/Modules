
#pragma once

#include "common.h"

namespace tp {

	extern ModuleManifest gModuleFilesystem;

	enum osfile_openflags {
		SAVE,
		LOAD,
	};

	class File {
		void* fh = NULL;

		public:
		alni adress = 0;
		bool opened = false;

		alni avl_adress = 0;
		
		void* preloaded = NULL;

		File();

		File(const char* path, osfile_openflags flags);

		void open(const char* path, osfile_openflags);

		void write_bytes(const int1* in, alni size, alni adress = -1);

		template <typename Type>
		void write(Type* in, alni adress = -1) {
			write_bytes((int1*) in, sizeof(Type), adress);
		}

		void read_bytes(int1* in, alni size, alni adress = -1);

		template <typename Type>
		void read(Type* in, alni adress = -1) {
			read_bytes((int1*) in, sizeof(Type), adress);
		}

		void write(const char* in1) {
			assert(0 && "depricated");
		}

		alni size();

		void fill(uint1 val, alni len);

		void close();

		void Preload();

		~File();

		alni sizeAllocatedMem();
		alni sizeUsedMem() {
			return sizeAllocatedMem();
		}

		static bool remove(const char* path);
		static bool rename(const char* path, const char* path2);
		static bool exists(const char* path);
	};

};