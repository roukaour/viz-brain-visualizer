#include <cctype>
#include <zlib.h>

#pragma warning(push, 0)
#include <FL/fl_utf8.h>
#include <FL/filename.H>
#pragma warning(pop)

#include "from-file.h"
#include "coords.h"
#include "input-parser.h"

const float Input_Parser::INVALID_FLOAT = 123456789.0f;

Input_Parser::Input_Parser() : From_File(), _file(NULL), _buffer_size(0), _buffer(NULL), _next(_buffer_size),
	_place(0), _overflow(false) {}

Input_Parser::Input_Parser(const char *f, size_t n) : From_File(), _file(NULL), _buffer_size(n), _buffer(NULL),
	_next(_buffer_size), _place(0), _overflow(false) {
	_filename = fl_filename_name(f);
	_filesize = ::filesize(f);
	_file = fl_fopen(f, "rb");
	_buffer = new(std::nothrow) char[_buffer_size];
}

Input_Parser::~Input_Parser() {
	if (_file) { fclose(_file); }
	delete [] _buffer;
}

int Input_Parser::first() {
	int c = next();
	while (IS_SPACE(c)) { c = next(); }
	while (c == INPUT_COMMENT_START) {
		for (c = next(); c != INPUT_COMMENT_END; c = next()) {}
		c = next();
		while (IS_SPACE(c)) { c = next(); }
	}
	return c;
}

int Input_Parser::next() {
	if (_next >= _buffer_size) {
		_next = 0;
		_buffer_size = fread(_buffer, 1, _buffer_size, _file);
		if (!_buffer_size) { return EOF; }
	}
	return _buffer[_next++];
}

size8_t Input_Parser::get_size8() {
	size8_t s = 0;
	int c = first();
	while (IS_DIGIT(c)) {
		s = (s * 10) + (size8_t)(c - '0');
		c = next();
	}
	if (c == INPUT_COMMENT_START) { _next--; }
	return s;
}

size16_t Input_Parser::get_size16() {
	size16_t s = 0;
	int c = first();
	while (IS_DIGIT(c)) {
		s = (s * 10) + (size16_t)(c - '0');
		c = next();
	}
	if (c == INPUT_COMMENT_START) { _next--; }
	return s;
}

size32_t Input_Parser::get_size32() {
	size32_t s = 0;
	int c = first();
	while (IS_DIGIT(c)) {
		s = (s * 10) + (size32_t)(c - '0');
		c = next();
	}
	if (c == INPUT_COMMENT_START) { _next--; }
	return s;
}

size64_t Input_Parser::get_size64() {
	size64_t s = 0;
	int c = first();
	while (IS_DIGIT(c)) {
		s = (s * 10) + (size64_t)(c - '0');
		c = next();
	}
	if (c == INPUT_COMMENT_START) { _next--; }
	return s;
}

int16_t Input_Parser::get_int16() {
	int32_t s = 0;
	int c = first();
	bool negative = false;
	if (c == '-') {
		negative = true;
		c = next();
	}
	while (IS_DIGIT(c)) {
		s = (s * 10) + (int32_t)(c - '0');
		c = next();
	}
	if (c == INPUT_COMMENT_START) { _next--; }
	if (negative) { s = -s; }
	return (int16_t)s;
}

float Input_Parser::get_float() {
	float f = 0.0f;
	int c = first();
	bool negative = false;
	if (c == '-') {
		negative = true;
		c = next();
	}
	while (IS_DIGIT(c)) {
		f = (f * 10.0f) + (float)(c - '0');
		c = next();
	}
	if (c == '.') {
		c = next();
		if (!IS_DIGIT(c)) {
			while (!IS_SPACE(c) && c != EOF) { c = next(); }
			return INVALID_FLOAT;
		}
		float d = 10.0f;
		while (IS_DIGIT(c)) {
			f += (float)(c - '0') / d;
			d *= 10.0f;
			c = next();
		}
	}
	else if (c == INPUT_COMMENT_START) { _next--; }
	else if (IS_SPACE(c) || c == EOF) {}
	else {
		while (!IS_SPACE(c) && c != EOF) { c = next(); }
		return INVALID_FLOAT;
	}
	if (negative) { f = -f; }
	return f;
}

double Input_Parser::get_double() {
	double f = 0.0;
	int c = first();
	bool negative = false;
	if (c == '-') {
		negative = true;
		c = next();
	}
	while (IS_DIGIT(c)) {
		f = (f * 10.0) + (double)(c - '0');
		c = next();
	}
	if (c == '.') {
		c = next();
		if (!IS_DIGIT(c)) {
			while (!IS_SPACE(c) && c != EOF) { c = next(); }
			return (double)INVALID_FLOAT;
		}
		double d = 10.0;
		while (IS_DIGIT(c)) {
			f += (double)(c - '0') / d;
			d *= 10.0;
			c = next();
		}
	}
	else if (c == INPUT_COMMENT_START) { _next--; }
	else if (IS_SPACE(c) || c == EOF) {}
	else {
		while (!IS_SPACE(c) && c != EOF) { c = next(); }
		return (double)INVALID_FLOAT;
	}
	if (negative) { f = -f; }
	return f;
}

coord_t Input_Parser::get_coord() {
#ifdef SHORT_COORDS
#define WIDE_COORD_T int32_t
#else
#define WIDE_COORD_T coord_t
#endif
	WIDE_COORD_T s = 0;
	int c = first();
	bool negative = false;
	if (c == '-') {
		negative = true;
		c = next();
	}
	while (IS_DIGIT(c)) {
		s = (s * 10) + (WIDE_COORD_T)(c - '0');
		c = next();
	}
	if (c == INPUT_COMMENT_START) { _next--; }
	if (negative) { s = -s; }
#ifdef SHORT_COORDS
	if (s < std::numeric_limits<coord_t>::min() || s > std::numeric_limits<coord_t>::max()) {
		_overflow = true;
	}
	return (coord_t)s;
#else
	return s;
#endif
#undef WIDE_COORD_T
}

Gzip_Input_Parser::Gzip_Input_Parser(const char *f, size_t n) : Input_Parser(), _gzfile(NULL) {
	_filename = fl_filename_name(f);
	_filesize = ::filesize(f);
	_gzfile = gzopen(f, "rb");
	_buffer_size = n;
	_buffer = new(std::nothrow) char[_buffer_size];
	_next = _buffer_size;
}

Gzip_Input_Parser::~Gzip_Input_Parser() {
	if (_gzfile) { gzclose(_gzfile); }
}

int Gzip_Input_Parser::next() {
	if (_next >= _buffer_size) {
		_next = 0;
		int n = gzread(_gzfile, _buffer, (unsigned int)_buffer_size);
		if (n <= 0) { return EOF; }
		_buffer_size = (size_t)n;
	}
	return _buffer[_next++];
}
