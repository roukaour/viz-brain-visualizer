#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <cstdio>
#include <cstdlib>
#include <zlib.h>

#include "utils.h"
#include "coords.h"
#include "from-file.h"

#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == '\v' || (c) == '\f')
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

#define INPUT_COMMENT_START '#'
#define INPUT_COMMENT_END '\n'

class Input_Parser : public From_File {
private:
	static const float INVALID_FLOAT;
protected:
	FILE *_file;
	size_t _buffer_size;
	char *_buffer;
	size_t _next;
	long _place;
	bool _overflow;
public:
	Input_Parser();
	Input_Parser(const char *f, size_t n = 8192);
	virtual ~Input_Parser();
	inline virtual bool good(void) const { return _file && _buffer && _buffer_size; }
	inline bool done(void) { return peek() == EOF; }
	inline int peek(void) { int c = first(); if (c != EOF) { _next--; } return c; }
	inline bool overflow(void) const { return _overflow; }
	inline virtual void save_place(void) { _place = (long)((size_t)ftell(_file) - _buffer_size + _next); }
	inline virtual void restore_place(void) { fseek(_file, _place, SEEK_SET); _next = _buffer_size; }
	inline bool get_bool(void) { return get_size32() > 0; }
	inline char get_char(void) { return (char)first(); }
	size8_t get_size8(void);
	size16_t get_size16(void);
	size32_t get_size32(void);
	size64_t get_size64(void);
	int16_t get_int16(void);
	float get_float(void);
	double get_double(void);
	coord_t get_coord(void);
protected:
	int first(void);
	virtual int next(void);
};

class Gzip_Input_Parser : public Input_Parser {
private:
	gzFile _gzfile;
public:
	Gzip_Input_Parser(const char *f, size_t n = 8192);
	virtual ~Gzip_Input_Parser();
	inline bool good(void) const { return _gzfile && _buffer && _buffer_size; }
	inline void save_place(void) { _place = (long)((size_t)gztell(_gzfile) - _buffer_size + _next); }
	inline void restore_place(void) { gzseek(_gzfile, _place, SEEK_SET); _next = _buffer_size; }
protected:
	int next(void);
};

#endif
