#include "SystemAPI.hpp"

#include <fstream>

using namespace tp;

ualni tp::FileSystemHandle::size() {
	auto strm = (std::fstream*) stream;
	ualni out = 0;
	strm->seekg(0, std::ios::beg);
	out = strm->tellg();
	strm->seekg(0, std::ios::end);
	out = (ualni) strm->tellg() - out;
	return out;
}

FileSystemHandle::FileSystemHandle() {
	stream = new std::fstream();
}

FileSystemHandle::~FileSystemHandle() {
	auto strm = (std::fstream*) stream;
	delete strm;
}

bool FileSystemHandle::isOpen() {
	auto strm = (std::fstream*) stream;
	return strm->is_open();
}

void FileSystemHandle::close() {
	auto strm = (std::fstream*) stream;
	strm->close();
}

void FileSystemHandle::seekp(ualni in) {
	auto strm = (std::fstream*) stream;
	strm->seekp(in);
}

void FileSystemHandle::read(int1* in, ualni size) {
	auto strm = (std::fstream*) stream;
	strm->read(in, size);
}

void FileSystemHandle::write(const int1* in, ualni size) {
	auto strm = (std::fstream*) stream;
	strm->write(in, size);
}

void FileSystemHandle::open(const char* path, bool read) {
	auto strm = (std::fstream*) stream;
	if (read)
		strm->open(path, std::ios::in | std::ios::binary | std::ios::app);
	else
		strm->open(path, std::ios::out | std::ios::binary | std::ios::trunc);
}
