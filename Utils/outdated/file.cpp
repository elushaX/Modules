
#include "env.h"

#include "file.h"

#include <fstream>
#include <string>
#include <iostream>
#include <cstdio>
#include <sys/stat.h>

typedef std::fstream osfstream;


#ifdef ENV_OS_ANDROID

const char* g_android_internal_data_path = "/";
const int max_len = 512;
char gbuff1[max_len];
char gbuff2[max_len];

// u own output untill next call to this function
const char* get_android_abs_path(const char* path, bool second = false) {

	auto buff = second ? gbuff2 : gbuff1;

	const int dir_len = strlen(g_android_internal_data_path);
	const int file_len = strlen(path);

	if (max_len < dir_len + file_len + 2) {
		return NULL;
	}

	memcpy(buff, g_android_internal_data_path, dir_len);
	buff[dir_len] = '/';

	memcpy(buff + dir_len + 1, path, file_len + 1);

	return buff;
}

#define PATH(__path__) get_android_abs_path(__path__)
#define PATH2(__path__) get_android_abs_path(__path__, true)

#else

#define PATH(__path__) __path__
#define PATH2(__path__) __path__

#endif

static inline bool file_exists(const char* name) {
	struct stat buffer;
	return (stat(name, &buffer) == 0);
}

static tp::ModuleManifest* sModuleDependencies[] = {
	&tp::gModuleCommon,
	NULL
};

namespace tp {

	ModuleManifest tp::gModuleFilesystem = ModuleManifest("FileSystem", NULL, NULL, sModuleDependencies);

	File::File() {
		MODULE_SANITY_CHECK(gModuleFilesystem);

		fh = NULL;
	}

	File::File(const char* path, osfile_openflags flags) {
		open(path, flags);
	}

	void File::open(const char* apath, osfile_openflags flags) {
		auto path = PATH(apath);

		close();

		adress = 0;
		avl_adress = 0;
		fh = new osfstream();
		osfstream& fhref = *((osfstream*) fh);

		switch (flags) {
			case SAVE:
				fhref.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
				break;

			case LOAD:
				//if (file_exists(path)) {
					fhref.open(path, std::ios::in | std::ios::binary | std::ios::app);
				//}
				break;
		};

		if (!fhref.is_open()) {
			opened = false;
			delete& fhref;
			return;
		}
		opened = true;
	}

	void File::close() {
		if (opened) {
			osfstream& fhref = *((osfstream*) fh);
			fhref.close();
			opened = false;
			delete ((osfstream*) fh);
		}
	}

	File::~File() {
		close();
		if (preloaded) {
			free(preloaded);
		}
	}

	void File::Preload() {
		if (!opened) return;

		RelAssert(!preloaded);
		alni filesize = size();
		preloaded = malloc(filesize);

		osfstream& fhref = *((osfstream*) fh);
		fhref.seekg(0);
		fhref.read((char*) preloaded, filesize);
	}

	alni File::sizeAllocatedMem() {
		alni out = 0;
		out += sizeof(osfstream);
		out += sizeof(alni) * 2;
		out += sizeof(bool) * 2;
		return out;
	}

	void File::write_bytes(const int1* in, alni size, alni p_adress) {
		if (!opened) return;

		if (p_adress == -1) {
			p_adress = this->adress;
			this->adress += size;
		}

		osfstream& fhref = *((osfstream*) fh);

		fhref.seekp(p_adress);
		fhref.write(in, size);
	}

	void File::read_bytes(int1* in, alni size, alni p_adress) {
		if (!opened) return;

		if (p_adress == -1) {
			p_adress = this->adress;
			adress += size;
		}

		osfstream& fhref = *((osfstream*) fh);

		fhref.seekg(p_adress);
		fhref.read(in, size);

		if (preloaded) {
			char* read_mem = ((char*) preloaded) + p_adress;
			memcp(in, read_mem, size);
		} else {
			osfstream& fhref = *((osfstream*) fh);
			fhref.seekg(p_adress);
			fhref.read(in, size);
		}
	}

	void File::fill(uint1 val, alni len) {
		if (!opened) return;

		for (int i = 0; i < len; i++) {
			write<uint1>(&val, i);
		}
	}

	alni File::size() {
		if (!opened) return 0;

		osfstream& fhref = *((osfstream*) fh);

		alni out = 0;
		fhref.seekg(0, std::ios::beg);
		out = fhref.tellg();
		fhref.seekg(0, std::ios::end);
		out = fhref.tellg() - out;
		return out;
	}

	bool File::remove(const char* path) {
		return (bool) std::remove(PATH(path));
	}

	bool File::rename(const char* path, const char* path2) {
		return (bool)std::rename(PATH(path), PATH2(path2));
	}

	bool File::exists(const char* path) {
		return file_exists(PATH(path));
	}
};