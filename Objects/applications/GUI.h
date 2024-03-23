
#pragma once

#include "primitives/PrimitiveObjects.hpp"
#include "primitives//InterpreterObject.hpp"

#include "compiler/Functions.hpp"

#include <string>

namespace obj {

	class ObjectsGUI {
		
		enum { MAX_NAME_LENGHT = 102 };
		char mNameEdit [MAX_NAME_LENGHT];
		obj::Object* mLastNameEditObject = NULL;

		struct ViewStackNode {
			obj::Object* obj;
			std::string id;
			ViewStackNode() { obj = NULL; }
			ViewStackNode(obj::Object* obj, const std::string& id) : obj(obj), id(id) {}
			operator bool() { return obj; }
		};

		tp::List<ViewStackNode> mViewStack;
		obj::Object* mRoot = NULL;
		obj::Object* mActive = NULL;
		obj::Object* mClipboard = NULL;

		void setClipboard(obj::Object*);
		obj::Object* getClipboard();

		void* mImGuiCtx = NULL;

	public:

		bool mShowDebugInfo = true;
		bool mCaptureInput = false;

		ObjectsGUI();
		~ObjectsGUI();

		void cd(obj::Object* child, const std::string& name);
		void cdup();

		void clearEvents();

		void draw();
		void explorer();
		void properties(const obj::ObjectType*, bool top_of_tree_vew = true);
		void drawFps();
	private:

		void preview(obj::Object* obj);
		ViewStackNode enumView(obj::EnumObject* obj);
		ViewStackNode boolView(obj::BoolObject* obj);
		ViewStackNode floatView(obj::FloatObject* obj);
		ViewStackNode nullView(obj::NullObject* obj);
		ViewStackNode linkoView(obj::LinkObject* obj);
		ViewStackNode intoView(obj::IntObject* obj);
		ViewStackNode dictView(obj::DictObject* obj);
		ViewStackNode listView(obj::ListObject* obj);
		ViewStackNode stringView(obj::StringObject* in);
		ViewStackNode classView(obj::ClassObject* in);
		ViewStackNode methodView(obj::MethodObject* in);
		ViewStackNode colorView(obj::ColorObject* in);
		ViewStackNode interpreterView(obj::InterpreterObject* in);
		void dictViewEdit(obj::DictObject* dict, const std::string& item_id, obj::Object* obj, bool& popup);
		void dictViewDrawCreate(obj::DictObject* dict);
	}; 
};
