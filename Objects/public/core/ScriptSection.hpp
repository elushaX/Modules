#pragma once

#include "primitives/MethodObject.hpp"

namespace tp::obj {

	// global singleton API for method object to manage the script
	struct ScriptSection {

		List<Script*> mScripts;
		
		Script* createScript();
		void changeScript(Script** current_script, Script** new_script);
		void abandonScript(Script* script);

		alni get_script_file_adress(Script* in);
		Script* get_scritp_from_file_adress(alni file_adress);

		~ScriptSection();

		static void initialize();
		static void uninitialize();
		static ScriptSection* globalHandle();

	private:

		static obj::save_load_callbacks slcb_script_section;

		void delete_script(Script* script);
		void reference_script(Script* script);

		static void save_script_table_to_file(ScriptSection* self, ArchiverOut& file);
		static void load_script_table_from_file(ScriptSection* self, ArchiverIn& file);
		static alni save_script_table_to_file_size(ScriptSection* self, ArchiverOut& file);
	};
}