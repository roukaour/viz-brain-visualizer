#include <cctype>
#include <string>
#include <limits>
#include <zlib.h>

#pragma warning(push, 0)
#include <FL/fl_utf8.h>
#include <FL/filename.H>
#pragma warning(pop)

#include "from-file.h"
#include "coords.h"
#include "binary-parser.h"

Binary_Parser::Binary_Parser() : From_File(), _file(NULL), _buffer_size(0), _buffer(NULL), _next(_buffer_size),
_place(0), _overflow(false) {}

Binary_Parser::Binary_Parser(const char *f, size_t n) : From_File(), _file(NULL), _buffer_size(n), _buffer(NULL),
_next(_buffer_size), _place(0), _overflow(false) {
	_filename = fl_filename_name(f);
	_filesize = ::filesize(f);
	_file = fl_fopen(f, "rb");
	_buffer = new(std::nothrow) unsigned char[_buffer_size];
}

Binary_Parser::~Binary_Parser() {
	if (_file) { fclose(_file); }
	delete [] _buffer;
}

int Binary_Parser::next() {
	if (_next >= _buffer_size) {
		_next = 0;
		_buffer_size = fread(_buffer, 1, _buffer_size, _file);
		if (!_buffer_size) { return EOF; }
	}
	return _buffer[_next++];
}

size64_t Binary_Parser::get_unsigned() {
	size64_t b0 = (size64_t)next();
	if (b0 == 0xFF) {
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		size64_t b3 = (size64_t)next();
		size64_t b4 = (size64_t)next();
		size64_t b5 = (size64_t)next();
		size64_t b6 = (size64_t)next();
		size64_t b7 = (size64_t)next();
		size64_t b8 = (size64_t)next();
		return (b1 << 56) | (b2 << 48) | (b3 << 40) | (b4 << 32) | (b5 << 24) | (b6 << 16) | (b7 << 8) | b8;
	}
	else if ((b0 & 0xFE) == 0xFE) {
		b0 -= 0xFE;
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		size64_t b3 = (size64_t)next();
		size64_t b4 = (size64_t)next();
		size64_t b5 = (size64_t)next();
		size64_t b6 = (size64_t)next();
		size64_t b7 = (size64_t)next();
		return (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) | (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
	}
	else if ((b0 & 0xFC) == 0xFC) {
		b0 -= 0xFC;
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		size64_t b3 = (size64_t)next();
		size64_t b4 = (size64_t)next();
		size64_t b5 = (size64_t)next();
		size64_t b6 = (size64_t)next();
		return (b0 << 48) | (b1 << 40) | (b2 << 32) | (b3 << 24) | (b4 << 16) | (b5 << 8) | b6;
	}
	else if ((b0 & 0xF8) == 0xF8) {
		b0 -= 0xF8;
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		size64_t b3 = (size64_t)next();
		size64_t b4 = (size64_t)next();
		size64_t b5 = (size64_t)next();
		return (b0 << 40) | (b1 << 32) | (b2 << 24) | (b3 << 16) | (b4 << 8) | b5;
	}
	else if ((b0 & 0xF0) == 0xF0) {
		b0 -= 0xF0;
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		size64_t b3 = (size64_t)next();
		size64_t b4 = (size64_t)next();
		return (b0 << 32) | (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
	}
	else if ((b0 & 0xE0) == 0xE0) {
		b0 -= 0xE0;
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		size64_t b3 = (size64_t)next();
		return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
	}
	else if ((b0 & 0xC0) == 0xC0) {
		b0 -= 0xC0;
		size64_t b1 = (size64_t)next();
		size64_t b2 = (size64_t)next();
		return (b0 << 16) | (b1 << 8) | b2;
	}
	else if ((b0 & 0x80) == 0x80) {
		b0 -= 0x80;
		size64_t b1 = (size64_t)next();
		return (b0 << 8) | b1;
	}
	else {
		return b0;
	}
}

int32_t Binary_Parser::get_signed() {
	size32_t b0 = (size32_t)next();
	if (b0 == 0xFF) {
		size32_t b1 = (size32_t)next();
		if (b1 & 0x80) {
			b1 -= 0x80;
			size32_t b2 = (size32_t)next();
			size32_t b3 = (size32_t)next();
			size32_t b4 = (size32_t)next();
			return -(int32_t)((b1 << 24) | (b2 << 16) | (b3 << 8) | b4);
		}
		else {
			size32_t b2 = (size32_t)next();
			size32_t b3 = (size32_t)next();
			size32_t b4 = (size32_t)next();
			return (int32_t)((b1 << 24) | (b2 << 16) | (b3 << 8) | b4);
		}
	}
	else if ((b0 & 0xFE) == 0xFE) {
		b0 -= 0xFE;
		size32_t b1 = (size32_t)next();
		size32_t b2 = (size32_t)next();
		size32_t b3 = (size32_t)next();
		return -(int32_t)((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
	}
	else if ((b0 & 0xFC) == 0xFC) {
		b0 -= 0xFC;
		size32_t b1 = (size32_t)next();
		size32_t b2 = (size32_t)next();
		size32_t b3 = (size32_t)next();
		return (int32_t)((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
	}
	else if ((b0 & 0xF8) == 0xF8) {
		b0 -= 0xF8;
		size32_t b1 = (size32_t)next();
		size32_t b2 = (size32_t)next();
		return -(int32_t)((b0 << 16) | (b1 << 8) | b2);
	}
	else if ((b0 & 0xF0) == 0xF0) {
		b0 -= 0xF0;
		size32_t b1 = (size32_t)next();
		size32_t b2 = (size32_t)next();
		return (int32_t)((b0 << 16) | (b1 << 8) | b2);
	}
	else if ((b0 & 0xE0) == 0xE0) {
		b0 -= 0xE0;
		size32_t b1 = (size32_t)next();
		return -(int32_t)((b0 << 8) | b1);
	}
	else if ((b0 & 0xC0) == 0xC0) {
		b0 -= 0xC0;
		size32_t b1 = (size32_t)next();
		return (int32_t)((b0 << 8) | b1);
	}
	else if ((b0 & 0x80) == 0x80) {
		b0 -= 0x80;
		return -(int32_t)b0;
	}
	else {
		return (int32_t)b0;
	}
}

coord_t Binary_Parser::get_coord(void) {
#ifdef SHORT_COORDS
	int32_t v = get_signed();
	if (v < std::numeric_limits<coord_t>::min() || v > std::numeric_limits<coord_t>::max()) {
		_overflow = true;
	}
	return (coord_t)v;
#else
	return (coord_t)get_signed();
#endif
}

std::string Binary_Parser::get_string() {
	std::string s;
	for (int c = next(); c != '\0' && c != EOF; c = next()) { s += (char)c; }
	return s;
}

void Binary_Parser::get_chars(size_t n, char *buffer) {
	for (size_t i = 0; i < n; i++) {
		buffer[i] = get_char();
	}
}

Gzip_Binary_Parser::Gzip_Binary_Parser(const char *f, size_t n) : Binary_Parser(), _gzfile(NULL) {
	_filename = fl_filename_name(f);
	_filesize = ::filesize(f);
	_gzfile = gzopen(f, "rb");
	_buffer_size = n;
	_buffer = new(std::nothrow) unsigned char[_buffer_size];
	_next = _buffer_size;
}

Gzip_Binary_Parser::~Gzip_Binary_Parser() {
	if (_gzfile) { gzclose(_gzfile); }
}

int Gzip_Binary_Parser::next() {
	if (_next >= _buffer_size) {
		_next = 0;
		int n = gzread(_gzfile, _buffer, (unsigned int)_buffer_size);
		if (n <= 0) { return EOF; }
		_buffer_size = (size_t)n;
	}
	return _buffer[_next++];
}
