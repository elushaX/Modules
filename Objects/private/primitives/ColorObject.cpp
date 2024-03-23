
#include "primitives/ColorObject.hpp"

using namespace obj;
using namespace tp;

void ColorObject::constructor(Object* self) { NDO_CAST(ColorObject, self)->mCol = tp::RGBA(1.f); }

void ColorObject::copy(ColorObject* self, const ColorObject* in) { self->mCol = in->mCol; }

ColorObject* ColorObject::create(tp::RGBA in) {
	NDO_CASTV(ColorObject, NDO->create("RGBA"), out)->mCol = in;
	return out;
}

void ColorObject::from_int(ColorObject* self, alni in) { self->mCol = tp::RGBA((tp::halnf) tp::clamp(in, (tp::alni) 0, (tp::alni) 1)); }

void ColorObject::from_float(ColorObject* self, alnf in) { NDO_CAST(ColorObject, self)->mCol = tp::RGBA(tp::clamp((tp::halnf) in, 0.f, 1.f)); }

std::string ColorObject::to_string(ColorObject* self) {
	// auto &col = NDO_CAST(ColorObject, self)->mCol;
	// return tp::sfmt("%i:%i:%i:%i", (tp::alni) (col.r * 255), (tp::alni)(col.g * 255), (tp::alni)(col.b * 255), (tp::alni)(col.a * 255));
	// TODO : implement
	return {};
}

static alni save_size(ColorObject* self) { return sizeof(tp::RGBA); }

static void save(ColorObject* self, ArchiverOut& file_self) { file_self << self->mCol; }

static void load(ArchiverIn& file_self, ColorObject* self) { file_self >> self->mCol; }

struct ObjectTypeConversions ColorObjectTypeConversions = {
	.from_int = (object_from_int) ColorObject::from_int,
	.from_float = (object_from_float) ColorObject::from_float,
	.from_string = nullptr,
	.to_string = (object_to_string) ColorObject::to_string,
	.to_int = nullptr,
	.to_float = nullptr,
};

void sub(ColorObject* self, ColorObject* in) { self->mCol = in->mCol - self->mCol; }

void add(ColorObject* self, ColorObject* in) { self->mCol = in->mCol + self->mCol; }

struct ObjectTypeAriphmetics ColorObject::TypeAriphm = {
	.add = (object_add) add,
	.sub = (object_sub) sub,
	.mul = (object_mul) nullptr,
	.div = (object_div) nullptr,
};

struct obj::ObjectType obj::ColorObject::TypeData = {
	.base = nullptr,
	.constructor = ColorObject::constructor,
	.destructor = nullptr,
	.copy = (object_copy) ColorObject::copy,
	.size = sizeof(ColorObject),
	.name = "RGBA",
	.convesions = &ColorObjectTypeConversions,
	.ariphmetics = &ColorObject::TypeAriphm,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
};
