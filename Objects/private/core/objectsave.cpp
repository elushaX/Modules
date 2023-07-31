

#include "NewPlacement.hpp"
#include "core/object.h"

#include "HeapAllocatorGlobal.hpp"

#include <malloc.h>

namespace obj {

	ObjectMemHead* bottom = nullptr;

	struct ObjectsFileHeader {
		char name[10] = {0};
		char version[10] = {0};

		ObjectsFileHeader(bool default_val = true) {
			if (default_val) {
				tp::memCopy(&name, "objects", tp::String::Logic::calcLength("objects") + 1);
				tp::memCopy(&version, "0", tp::String::Logic::calcLength("0") + 1);
			}
		}
	};


	Object* ObjectMemAllocate(const ObjectType* type) {
		ObjectMemHead* memh = (ObjectMemHead*) tp::HeapAllocGlobal::allocate(type->size + sizeof(ObjectMemHead));
		if (!memh) {
			return NULL;
		}

		memh->down = NULL;
		memh->flags = 0;

		#ifdef OBJECT_REF_COUNT
		memh->refc = (tp::alni) 1;
		#endif

		if (bottom) {
			bottom->down = memh;
		}
		memh->up = bottom;
		bottom = memh;

		NDO_FROM_MEMH(memh)->type = type;
		return NDO_FROM_MEMH(memh);
	}

	void ObjectMemDeallocate(Object* in) {
		ObjectMemHead* memh = ((ObjectMemHead*) in) - 1;

		if (memh->up) {
			memh->up->down = memh->down;
		}

		if (memh->down) {
			memh->down->up = memh->up;
		} else {
			bottom = memh->up;
		}

		tp::HeapAllocGlobal::deallocate(memh);
	}

	struct ObjectFileHead {
		Object* load_head_adress = 0;
		tp::halni refc = 0;
	};

	tp::int1* loaded_file = nullptr;


	void objects_api::clear_object_flags() {
		// clear all object flags
		for (ObjectMemHead* iter = bottom; iter; iter = iter->up) {
			iter->flags = -1;
		}
	}

	tp::alni& getObjectFlags(Object* in) {
		return NDO_MEMH_FROM_NDO(in)->flags;
	}

	tp::alni objsize_ram_util(Object* self, const ObjectType* type) {
		tp::alni out = 0;

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

	tp::alni objects_api::objsize_ram(Object* self) {
		return objsize_ram_util(self, self->type);
	}

	tp::alni objects_api::objsize_ram_recursive_util(Object* self, const ObjectType* type, bool different_object) {
		tp::alni out = 0;

		if (different_object) {
			if (getObjectFlags(self) == 1) {
				return 0;
			}

			getObjectFlags(self) = 1;
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

	tp::alni objects_api::objsize_ram_recursive(Object* self) {
		clear_object_flags();
		return objsize_ram_recursive_util(self, self->type);
	}

	tp::alni objsize_file_util(Object* self, const ObjectType* type) {
		tp::alni out = 0;

		if (type->save_size) {
			out += type->save_size(self);
		}

		if (type->base) {
			out += objsize_file_util(self, type->base);
		}
		return out;
	}

	tp::alni objects_api::objsize_file(Object* self) {
		return objsize_file_util(self, self->type);
	}

	tp::alni objsize_file_recursive_util(Object* self, const ObjectType* type) {
		tp::alni out = 0;

		getObjectFlags(self) = 1;

		if (type->save_size) {
			out += type->save_size(self);
		}

		if (type->childs_retrival) {
			tp::Buffer<Object*> childs = type->childs_retrival(self);
			for (auto child : childs) {
				if (getObjectFlags(child.data()) != 1) {
					out += objsize_file_recursive_util(child.data(), child.data()->type);
				}
			}
		}

		if (type->base) {
			out += objsize_file_recursive_util(self, type->base);
		}

		return out;
	}

	tp::alni objects_api::objsize_file_recursive(Object* self) {
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

	tp::alni objects_api::save(ArchiverOut& ndf, Object* in) {
		// if already saved return file_adress
		if (NDO_MEMH_FROM_NDO(in)->flags != -1) {
			return NDO_MEMH_FROM_NDO(in)->flags;
		}

		// save write adress for parent save function call 
		tp::alni tmp_adress = ndf.getAddress();

		// save requested object to first available adress
		tp::alni save_adress = ndf.getFreeAddress();

		// save file_adress in memhead
		NDO_MEMH_FROM_NDO(in)->flags = save_adress;

		// update write adress
		ndf.setAddress(save_adress);

		// save file object header
		ObjectFileHead ofh = { 0, getrefc(in) };
		ndf << ofh;
		ndf << tp::String(in->type->name);

		// allocate for object file header
		ndf.setFreeAddress(ndf.getFreeAddress() + sizeof(ObjectFileHead) + tp::String::Logic::calcLength(in->type->name) + sizeof(tp::String::Logic::Index));

		// calc max size needed for saving all hierarchy of types
		tp::alni file_alloc_size = objsize_file_util(in, in->type);

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

	Object* objects_api::load(ArchiverIn& ndf, tp::alni file_adress) {
		// check if already saved
		if (((ObjectFileHead*) (loaded_file + file_adress))->load_head_adress) {
			return ((ObjectFileHead*) (loaded_file + file_adress))->load_head_adress;
		}

		// save read address
		tp::alni parent_file_adress = ndf.getAddress();

		// set read address
		ndf.setAddress(file_adress);

		ObjectFileHead ofh;
		ndf >> ofh;

		tp::String type_name;
		ndf >> type_name;

		const ObjectType* object_type = NDO->types.get(type_name);
		Object* out = ObjectMemAllocate(object_type);

		if (!out) {
			return NULL;
		}

		setrefc(out, ofh.refc);

		// save heap adress in "loaded_file"
		((ObjectFileHead*) (loaded_file + file_adress))->load_head_adress = out;

		// loads recursively
		object_recursive_load(ndf, out, object_type);

		// restore read address for parent call to continue
		ndf.setAddress(parent_file_adress);

		// return heap memory adress
		return out;
	}

	bool objects_api::save(Object* in, tp::String path, bool compressed) {
		ArchiverOut ndf(path.read());

		if (!ndf.isOpened()) {
			return false;
		}

		clear_object_flags();

		// save version info
		ObjectsFileHeader header;
		ndf << header;

		ndf.setFreeAddress(ndf.getAddress());

		// pre allocate
		for (tp::alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
			if (sl_callbacks[i]) {
				DEBUG_ASSERT(sl_callbacks[i]->size);
				ndf.setFreeAddress(ndf.getFreeAddress() + sl_callbacks[i]->size(sl_callbacks[i]->self, ndf));
			}
		}

		// pre-save
		for (tp::alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
			if (sl_callbacks[i] && sl_callbacks[i]->pre_save) {
				sl_callbacks[i]->pre_save(sl_callbacks[i]->self, ndf);
			}
		}

		save(ndf, in);

		// post-save
		for (tp::alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
			if (sl_callbacks[i] && sl_callbacks[i]->post_save) {
				sl_callbacks[i]->post_save(sl_callbacks[i]->self, ndf);
			}
		}

		// TODO : add compression
		/*
		if (compressed) {
			auto temp = path + ".bz2";
			tp::compressF2F(path.read(), temp.cstr());

			File::remove(path.read());
			File::rename(temp.cstr(), path.read());
		}
		*/

		return true;
	}

	Object* objects_api::load(tp::String path) {
		/*
		auto temp_file_name = path + ".unz";
		bool unz_res = tp::decompressF2F(path.cstr(), temp_file_name.cstr());

		if (!unz_res) {
			return NULL;
		}

		File ndf(temp_file_name.cstr(), tp::osfile_openflags::LOAD);
	 	*/

		ArchiverIn ndf(path.read());

		if (!ndf.isOpened()) {
			return NULL;
		}

		// check for compatibility
		ObjectsFileHeader current_header;
		ObjectsFileHeader loaded_header(false);

		ndf >> loaded_header;

		if (!tp::memEqual(&current_header, &loaded_header, sizeof(ObjectsFileHeader))) {
			return NULL;
		}

		ndf.setAddress(0);

		const auto fileSize = ndf.getSize();
		loaded_file = (tp::int1*) malloc(fileSize);
		tp::memSetVal(loaded_file, fileSize, 0);
		ndf.readBytes(loaded_file, fileSize);

		ndf.setAddress(sizeof(ObjectsFileHeader));

		// preload
		for (tp::alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
			if (sl_callbacks[i] && sl_callbacks[i]->pre_load) {
				sl_callbacks[i]->pre_load(sl_callbacks[i]->self, ndf);
			}
		}

		Object* out = load(ndf, ndf.getAddress());

		// post
		for (tp::alni i = 0; i < SAVE_LOAD_MAX_CALLBACK_SLOTS; i++) {
			if (sl_callbacks[i] && sl_callbacks[i]->post_save) {
				sl_callbacks[i]->post_load(sl_callbacks[i]->self, ndf);
			}
		}

		free(loaded_file);

		/*
		ndf.close();

		if (unz_res == NULL) {
			File::remove(temp_file_name.cstr());
		}
		*/

		setrefc(out, 0);

		return out;
	}

	void objects_api::add_sl_callbacks(save_load_callbacks* in) {
		sl_callbacks[sl_callbacks_load_idx] = in;
		sl_callbacks_load_idx++;
	}
};