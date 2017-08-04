#ifndef BINARY_PARSER_H
#define BINARY_PARSER_H

#include <cstdio>
#include <cstdlib>
#include <string>
#include <zlib.h>

#include "utils.h"
#include "coords.h"
#include "from-file.h"

class Binary_Parser : public From_File {
protected:
	FILE *_file;
	size_t _buffer_size;
	unsigned char *_buffer;
	size_t _next;
	long _place;
	bool _overflow;
public:
	Binary_Parser();
	Binary_Parser(const char *f, size_t n = 8192);
	virtual ~Binary_Parser();
	inline virtual bool good(void) const { return _file && _buffer && _buffer_size; }
	inline bool done(void) { return peek() == EOF; }
	inline int peek(void) { int c = next(); if (c != EOF) { _next--; } return c; }
	inline bool overflow(void) const { return _overflow; }
	inline virtual void save_place(void) { _place = (long)((size_t)ftell(_file) - _buffer_size + _next); }
	inline virtual void restore_place(void) { fseek(_file, _place, SEEK_SET); _next = _buffer_size; }
	inline bool get_bool(void) { return get_size8() > 0; }
	inline char get_char(void) { return (char)next(); }
	inline size8_t get_size8(void) { return (size8_t)next(); }
	inline size16_t get_size16(void) { return (size16_t)get_unsigned(); }
	inline size32_t get_size32(void) { return (size32_t)get_unsigned(); }
	inline size64_t get_size64(void) { return (size64_t)get_unsigned(); }
	size64_t get_unsigned(void);
	inline int16_t get_int16(void) { return (int16_t)get_signed(); }
	inline int32_t get_int32(void) { return (int32_t)get_signed(); }
	coord_t get_coord(void);
	int32_t get_signed(void);
	std::string get_string(void);
	void get_chars(size_t n, char *buffer);
private:
	virtual int next(void);
};

class Gzip_Binary_Parser : public Binary_Parser {
private:
	gzFile _gzfile;
public:
	Gzip_Binary_Parser(const char *f, size_t n = 8192);
	virtual ~Gzip_Binary_Parser();
	inline bool good(void) const { return _gzfile && _buffer && _buffer_size; }
	inline void save_place(void) { _place = (long)((size_t)gztell(_gzfile) - _buffer_size + _next); }
	inline void restore_place(void) { gzseek(_gzfile, _place, SEEK_SET); _next = _buffer_size; }
protected:
	int next(void);
};

#endif
