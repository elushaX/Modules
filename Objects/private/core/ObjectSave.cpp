

#include "core/Object.hpp"

#include "primitives/NullObject.hpp"

#include <malloc.h>

using namespace tp;
using namespace obj;

Object* bottom = nullptr;
ualni count = 0;

Object* ObjectMemAllocate(const ObjectType* type) {
	auto object = (Object*) malloc(type->size);

	if (!object) {
		printf("Cant allocate memory for an object");
		exit(0);
	}

	object->down = nullptr;
	object->flags = 0;

	object->references = (alni) 1;

	if (bottom) {
		bottom->down = object;
	}

	object->up = bottom;
	bottom = object;

	count++;

	object->type = type;

	return object;
}

void ObjectMemDeallocate(Object* object) {
	if (object->up) {
		object->up->down = object->down;
	}

	if (object->down) {
		object->down->up = object->up;
	} else {
		bottom = object->up;
	}

	free(object);
	count--;
}

void objects_api::logTypeData(const ObjectType* type) {
	printf("type - %s\n", type->name);
	if (type->base) {
		printf("Based on ");
		logTypeData(type->base);
	}
}

ualni objects_api::getObjCount() { return count; }

void objects_api::assertNoLeaks() {
	if (bottom) {
		printf("ERROR : not all objects are destroyed\n");
		ualni idx = 0;
		for (Object* object = bottom; object; object = object->up) {
			printf(" ===== Object - %llu. Ref count - %lli ===== \n", idx, object->references);
			logTypeData(object->type);
			idx++;
		}
	}
}

struct ObjectsFileHeader {
	char name[10] = { 0 };
	char version[10] = { 0 };

	explicit ObjectsFileHeader(bool default_val = true) {
		if (default_val) {
			memCopy(&name, "objects", 8);
			memCopy(&version, "0", 2);
		}
	}
};

struct ObjectFileHead {
	Object* load_head_adress = nullptr;
	halni refc = 0;
};

int1* loaded_file = nullptr;

void objects_api::clear_object_flags() {
	// clear all object flags
	for (Object* iter = bottom; iter; iter = iter->up) {
		iter->flags = -1;
	}
}

alni objsize_ram_util(Object* self, const ObjectType* type) {
	alni out = 0;

	if (type->allocated_size) {
		out += type->allocated_size(self);
	} else {
		out += type->size - sizeof(ObjectType*);
	}

	if (type->base) {
		out += objsize_ram_util(self, type->base);
	} else {
		out += sizeof(ObjectType*);
	}

	return out;
}

alni objects_api::objsize_ram(Object* self) { return objsize_ram_util(self, self->type); }

alni objects_api::objsize_ram_recursive_util(Object* self, const ObjectType* type, bool different_object) {
	alni out = 0;

	if (different_object) {
		if (self->flags == 1) {
			return 0;
		}

		self->flags = 1;
	}

	if (type->allocated_size_recursive) {
		out += type->allocated_size_recursive(self);
	} else {
		if (type->allocated_size) {
			out += type->allocated_size(self);
		} else {
			out += type->size - sizeof(ObjectType*);
		}
	}

	if (type->base) {
		out += objsize_ram_recursive_util(self, type->base, false);
	} else {
		out += sizeof(ObjectType*);
	}

	return out;
}

alni objects_api::objsize_ram_recursive(Object* self) {
	clear_object_flags();
	return objsize_ram_recursive_util(self, self->type);
}

alni objsize_file_util(Object* self, const ObjectType* type) {
	alni out = 0;

	if (type->save_size) {
		out += type->save_size(self);
	}

	if (type->base) {
		out += objsize_file_util(self, type->base);
	}
	return out;
}

alni objects_api::objsize_file(Object* self) { return objsize_file_util(self, self->type); }

alni objsize_file_recursive_util(Object* self, const ObjectType* type) {
	alni out = 0;

	self->flags = 1;

	if (type->save_size) {
		out += type->save_size(self);
	}

	if (type->childs_retrival) {
		Buffer<Object*> childs = type->childs_retrival(self);
		for (auto child : childs) {
			if (child.data()->flags != 1) {
				out += objsize_file_recursive_util(child.data(), child.data()->type);
			}
		}
	}

	if (type->base) {
		out += objsize_file_recursive_util(self, type->base);
	}

	return out;
}

alni objects_api::objsize_file_recursive(Object* self) {
	clear_object_flags();
	return objsize_file_recursive_util(self, self->type);
}

void object_recursive_save(ArchiverOut& ndf, Object* self, const ObjectType* type) {
	if (type->base) {
		object_recursive_save(ndf, self, type->base);
	}

	// automatically offsets for parent type to write
	if (type->save) {
		type->save(self, ndf);
	}
}

alni objects_api::save(ArchiverOut& ndf, Object* in) {
	// if already saved return file_adress
	if (in->flags != -1) {
		return in->flags;
	}

	// save write adress for parent save function call
	alni tmp_adress = ndf.getAddress();

	// save requested object to first available adress
	alni save_adress = ndf.getFreeAddress();

	// save file_adress in memhead
	in->flags = save_adress;

	// update write adress
	ndf.setAddress(save_adress);

	// save file object header
	ObjectFileHead ofh = { 0, (halni) getReferenceCount(in) };
	ndf << ofh;

	save_string(ndf, in->type->name);

	// allocate for object file header
	ndf.setFreeAddress(ndf.getFreeAddress() + sizeof(ObjectFileHead) + save_string_size(in->type->name));

	// calc max size needed for saving all hierarchy of types
	alni file_alloc_size = objsize_file_util(in, in->type);

	// offes first available adress
	ndf.setFreeAddress(ndf.getFreeAddress() + file_alloc_size);

	object_recursive_save(ndf, in, in->type);

	// restore adress for parent save function
	ndf.setAddress(tmp_adress);

	// return addres of saved object in file space
	return save_adress;
}

void object_recursive_load(ArchiverIn& ndf, Object* out, const ObjectType* type) {
	if (type->base) {
		object_recursive_load(ndf, out, type->base);
	}

	// automatically offsets for parent type to read
	if (type->load) {
		type->load(ndf, out);
	}
}

Object* objects_api::load(ArchiverIn& ndf, alni file_adress) {
	// check if already saved
	if (((ObjectFileHead*) (loaded_file + file_adress))->load_head_adress) {
		return ((ObjectFileHead*) (loaded_file + file_adress))->load_head_adress;
	}

	// save read address
	alni parent_file_adress = ndf.getAddress();

	// set read address
	ndf.setAddress(file_adress);

	ObjectFileHead ofh;
	ndf >> ofh;

	std::string type_name;
	load_string(ndf, type_name);

	const ObjectType* object_type = gObjectsContext->types.get(type_name);
	Object* out = ObjectMemAllocate(object_type);

	if (!out) {
		return nullptr;
	}

	setReferenceCount(out, 0);

	// check for null object
	if (out->type == &NullObject::TypeData) {
		ObjectMemDeallocate(out);
		out = NdoNull_globalInstance;
	}

	// save heap adress in "loaded_file"
	((ObjectFileHead*) (loaded_file + file_adress))->load_head_adress = out;

	// loads recursively
	object_recursive_load(ndf, out, object_type);

	// restore read address for parent call to continue
	ndf.setAddress(parent_file_adress);

	// return heap memory adress
	return out;
}

bool objects_api::save(Object* in, const std::string& path, bool compressed) {
	ArchiverOut ndf(path.c_str());

	if (!ndf.isOpened()) {
		return false;
	}

	clear_object_flags();

	// save version info
	ObjectsFileHeader header;
	ndf << header;

	ndf.setFreeAddress(ndf.getAddress());

	// pre allocate
	for (alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
		if (gObjectsContext->sl_callbacks[i]) {
			DEBUG_ASSERT(gObjectsContext->sl_callbacks[i]->size);
			ndf.setFreeAddress(
				ndf.getFreeAddress() + gObjectsContext->sl_callbacks[i]->size(gObjectsContext->sl_callbacks[i]->self, ndf)
			);
		}
	}

	// pre-save
	for (alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
		if (gObjectsContext->sl_callbacks[i] && gObjectsContext->sl_callbacks[i]->pre_save) {
			gObjectsContext->sl_callbacks[i]->pre_save(gObjectsContext->sl_callbacks[i]->self, ndf);
		}
	}

	save(ndf, in);

	// post-save
	for (alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
		if (gObjectsContext->sl_callbacks[i] && gObjectsContext->sl_callbacks[i]->post_save) {
			gObjectsContext->sl_callbacks[i]->post_save(gObjectsContext->sl_callbacks[i]->self, ndf);
		}
	}

	// TODO : add compression
	/*
	if (compressed) {
		auto temp = path + ".bz2";
		compressF2F(path.read(), temp.cstr());

		File::remove(path.read());
		File::rename(temp.cstr(), path.read());
	}
	*/

	return true;
}

Object* objects_api::load(const std::string& path) {
	/*
	auto temp_file_name = path + ".unz";
	bool unz_res = decompressF2F(path.cstr(), temp_file_name.cstr());

	if (!unz_res) {
		return nullptr;
	}

	File ndf(temp_file_name.cstr(), osfile_openflags::LOAD);
	*/

	ArchiverIn ndf(path.c_str());

	if (!ndf.isOpened()) {
		return nullptr;
	}

	// check for compatibility
	ObjectsFileHeader current_header;
	ObjectsFileHeader loaded_header(false);

	ndf >> loaded_header;

	if (!memEqual(&current_header, &loaded_header, sizeof(ObjectsFileHeader))) {
		return nullptr;
	}

	ndf.setAddress(0);

	const auto fileSize = ndf.getSize();
	loaded_file = (int1*) malloc(fileSize);
	memSetVal(loaded_file, fileSize, 0);
	ndf.readBytes(loaded_file, fileSize);

	ndf.setAddress(sizeof(ObjectsFileHeader));

	// preload
	for (alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
		if (gObjectsContext->sl_callbacks[i] && gObjectsContext->sl_callbacks[i]->pre_load) {
			gObjectsContext->sl_callbacks[i]->pre_load(gObjectsContext->sl_callbacks[i]->self, ndf);
		}
	}

	Object* out = load(ndf, ndf.getAddress());

	// post
	for (alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
		if (gObjectsContext->sl_callbacks[i] && gObjectsContext->sl_callbacks[i]->post_save) {
			gObjectsContext->sl_callbacks[i]->post_load(gObjectsContext->sl_callbacks[i]->self, ndf);
		}
	}

	free(loaded_file);

	/*
	ndf.close();

	if (unz_res == nullptr) {
		File::remove(temp_file_name.cstr());
	}
	*/

	return out;
}

void objects_api::add_sl_callbacks(save_load_callbacks* in) {
	gObjectsContext->sl_callbacks[gObjectsContext->sl_callbacks_load_idx] = in;
	gObjectsContext->sl_callbacks_load_idx++;
}
