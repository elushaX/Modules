
#pragma once

#include "primitives/primitives.h"
#include "primitives//interpreterobject.h"

#include "compiler/function.h"

#include "strings.h"

namespace obj {

	class ObjectsGUI {
		
		enum { MAX_NAME_LENGHT = 102 };
		char mNameEdit [MAX_NAME_LENGHT];
		obj::Object* mLastNameEditObject = NULL;

		struct ViewStackNode {
			obj::Object* obj;
			tp::String id;
			ViewStackNode() { obj = NULL; }
			ViewStackNode(obj::Object* obj, tp::String id) : obj(obj), id(id) {}
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

		void cd(obj::Object* child, tp::String name);
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
		void dictViewEdit(obj::DictObject* dict, tp::String item_id, obj::Object* obj, bool& popup);
		void dictViewDrawCreate(obj::DictObject* dict);
	}; 
};
