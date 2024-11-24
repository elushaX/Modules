
#include "strokesobject.h"

using namespace tp;
using namespace obj;

void StrokesObject::constructor(StrokesObject* self) {
	new (&self->mProject) strokes::Project();
}

void StrokesObject::copy(StrokesObject* self, const StrokesObject* in) {
	self->mProject = in->mProject;
}

void StrokesObject::destructor(StrokesObject* self) {
	self->mProject.~Project();
}

static alni save_size(StrokesObject* self) {
	return self->mProject.saveSize();
}

static void save(StrokesObject* self, File& file_self) {
	self->mProject.save(file_self);
}

static void load(File& file_self, StrokesObject* self) {
	new (&self->mProject) strokes::Project();
	self->mProject.load(file_self);
}

struct ObjectType obj::StrokesObject::TypeData = {
	.base = NULL,
	.constructor = (object_constructor) StrokesObject::constructor,
	.destructor = (object_destructor) StrokesObject::destructor,
	.copy = (object_copy) StrokesObject::copy,
	.size = sizeof(StrokesObject),
	.name = "StrokesProject",
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
};
