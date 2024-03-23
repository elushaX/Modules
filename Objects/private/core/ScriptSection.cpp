
#include "core/ScriptSection.hpp"

using namespace tp;
using namespace obj;

ScriptSection* gScriptSection = nullptr;

struct script_data_head {
	alni refc = 0;
	alni store_adress = 0;
};

void set_script_head_store_adress(Script* in, alni address) { ((script_data_head*) in - 1)->store_adress = address; }

alni get_script_head_store_adress(Script* in) { return ((script_data_head*) in - 1)->store_adress; }

Script* ScriptSection::createScript() {
	auto sdhead = (script_data_head*) malloc(sizeof(script_data_head) + sizeof(Script));
	auto out = (Script*) (sdhead + 1);

	if (!sdhead) {
		return nullptr;
	}

	sdhead->refc = 1;
	sdhead->store_adress = -1;

	mScripts.pushBack(out);

	new (out) Script();
	out->mReadable = obj::StringObject::create("");
	obj::NDO->increaseReferenceCount(out->mReadable);
	return out;
}

void ScriptSection::delete_script(Script* script) {
	script->~Script();
	obj::NDO->destroy(script->mReadable);

	free((((script_data_head*) script) - 1));
}

void ScriptSection::reference_script(Script* script) { (((script_data_head*) script) - 1)->refc++; }

void ScriptSection::abandonScript(Script* script) {
	if ((((script_data_head*) script) - 1)->refc > 1) {
		(((script_data_head*) script) - 1)->refc--;
	} else {
		// ISSUE : on delete list structure is outdated
		// FIXME : use pool alloc with ref count
		// forwarding to global destruction
		// delete_script(script);
	}
}

void ScriptSection::changeScript(Script** current_script, Script** new_script) {
	abandonScript(*current_script);
	reference_script(*new_script);

	*current_script = *new_script;
}

alni ScriptSection::get_script_file_adress(Script* in) { return get_script_head_store_adress(in); }

alni ScriptSection::save_script_table_to_file_size(ScriptSection* self, ArchiverOut& file) {
	alni size = 0;
	size += 5;                // header
	size += sizeof(alni); // scripts length

	for (auto iter : self->mScripts) {

		set_script_head_store_adress(iter.data(), file.getAddress() + size);

		size += sizeof(alni); // scripts string obj ref
		size += sizeof(alni); // constants length

		for (auto const_obj : iter->mBytecode.mConstants) {
			size += sizeof(alni); // constant object addres
		}

		// instructions
		size += SaveSizeCounter::calc(iter->mBytecode.mInstructions);
	}

	size += sizeof(alni); // objects mem offset
	size += 5;                // header
	return size;
}

void ScriptSection::save_script_table_to_file(ScriptSection* self, ArchiverOut& file) {
	// header
	file.writeBytes("/scr/", 5);

	// scripts length
	alni scripts_count = self->mScripts.length();
	file << scripts_count;

	for (auto iter : self->mScripts) {

		// scripts string obj ref
		auto obj_addres = obj::NDO->save(file, iter->mReadable);
		file << obj_addres;

		// constants length
		alni consts_count = iter->mBytecode.mConstants.size();
		file << consts_count;

		for (auto const_obj : iter->mBytecode.mConstants) {
			// constant object addres
			auto obj_addres = obj::NDO->save(file, const_obj.data());
			file << obj_addres;
		}

		// mInstructions
		file << iter->mBytecode.mInstructions;
	}

	// header
	file.writeBytes("/scr/", 5);

	alni objects_mem_offset = file.getFreeAddress();
	file << objects_mem_offset;
}

void load_constants(ScriptSection* self, ArchiverIn& file, alni start_addr) {
	auto addres = file.getAddress();

	file.setAddress(start_addr);
	file.setAddress(start_addr + 5); // header

	alni scripts_count;
	file >> scripts_count;

	for (alni i = 0; i < scripts_count; i++) {

		auto script = self->get_scritp_from_file_adress(file.getAddress());

		// script text
		alni str_addr;
		file >> str_addr;

		NDO->destroy(script->mReadable); // we already have string object in the script when creating script
		script->mReadable = NDO_CAST(obj::StringObject, obj::NDO->load(file, str_addr));

		file.setAddress(file.getAddress() + sizeof(alni)); // constants length

		for (auto const_obj : script->mBytecode.mConstants) {
			alni consts_addr;
			file >> consts_addr;
			const_obj.data() = obj::NDO->load(file, consts_addr);
		}

		// instructions
		file.setAddress(file.getAddress() + SaveSizeCounter::calc(script->mBytecode.mInstructions));
	}

	file.setAddress(addres);
}

void ScriptSection::load_script_table_from_file(ScriptSection* self, ArchiverIn& file) {
	for (auto iter : self->mScripts) {
		set_script_head_store_adress(iter.data(), -1);
	}

	auto section_start_addr = file.getAddress();

	// header
	file.setAddress(file.getAddress() + 5);

	// scripts length
	alni scripts_count;
	file >> scripts_count;

	for (alni i = 0; i < scripts_count; i++) {

		auto new_script = self->createScript();
		set_script_head_store_adress(new_script, file.getAddress());

		// scripts text
		file.setAddress(file.getAddress() + sizeof(alni));

		// constants length
		alni consts_count;
		file >> consts_count;

		new_script->mBytecode.mConstants.reserve(consts_count);

		for (alni j = 0; j < consts_count; j++) {
			// constant object addres
			alni consts_addr;
			file >> consts_addr;

			// skiping
			// new_script->mBytecode.mConstants[j] = obj::NDO->load(file, consts_addr);
		}

		// mInstructions
		file >> new_script->mBytecode.mInstructions;
	}

	load_constants(self, file, section_start_addr);

	// header
	file.setAddress(file.getAddress() + 5);

	alni objects_mem_offset;
	file >> objects_mem_offset;

	file.setAddress(objects_mem_offset);
}

Script* ScriptSection::get_scritp_from_file_adress(alni file_adress) {
	for (auto iter : mScripts) {
		if (get_script_head_store_adress(iter.data()) == file_adress) {
			return iter.data();
		}
	}
	return nullptr;
}

ScriptSection::~ScriptSection() {
	for (auto iter : mScripts) {
		delete_script(iter.data());
	}
}

obj::save_load_callbacks ScriptSection::slcb_script_section = {
	.self = gScriptSection,
	.pre_save = (obj::pre_save_callback*) ScriptSection::save_script_table_to_file,
	.size = (obj::slcb_size_callback*) ScriptSection::save_script_table_to_file_size,
	.pre_load = (obj::pre_load_callback*) ScriptSection::load_script_table_from_file,
	.post_save = nullptr,
	.post_load = nullptr,
};

void ScriptSection::initialize() {
	ASSERT(!gScriptSection);
	gScriptSection = new ScriptSection();

	ASSERT(obj::NDO && "Forgot to initialize objects?");

	slcb_script_section.self = gScriptSection;
	obj::NDO->add_sl_callbacks(&slcb_script_section);
}

void ScriptSection::uninitialize() {
	ASSERT(gScriptSection);
	delete gScriptSection;
	gScriptSection = nullptr;
}

ScriptSection* ScriptSection::globalHandle() { return gScriptSection; }
