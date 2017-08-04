#ifndef FROM_FILE_H
#define FROM_FILE_H

#include <string>

enum Read_Status {
	SUCCESS, FAILURE, CANCELED, END_OF_FILE, SIGN_OVERFLOW, NO_MEMORY, NO_TYPES, NO_SOMAS, NO_CYCLES, WRONG_NUM_SOMAS,
	WRONG_NUM_SYNAPSES, WRONG_NUM_CYCLES, BAD_TYPE_LETTER, BAD_SOMA_ID, BAD_SYNAPSE_SOMA_ID, BAD_GAP_JUNCTION_SOMA_ID,
	BAD_SIGNATURE, BAD_VERSION
};

class From_File {
protected:
	std::string _filename;
	size_t _filesize;
public:
	From_File(const char *f = NULL, size_t z = 0);
	virtual ~From_File();
	std::string filename(void) const { return _filename; }
	void filename(const char *f) { _filename = f; }
	size_t filesize(void) const { return _filesize; }
	void filesize(size_t z) { _filesize = z; }
	void print(std::ostream &os) const;
};

size_t filesize(const char *f);
std::string read_status_message(Read_Status status, const char *filename);

#endif
