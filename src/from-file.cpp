#include <iostream>
#include <sys/stat.h>

#include "from-file.h"

From_File::From_File(const char *f, size_t z) : _filename(f ? f : ""), _filesize(z) {}

From_File::~From_File() {
	_filename = "";
	_filesize = 0;
}

void From_File::print(std::ostream &os) const {
	os << _filename.c_str() << " (";
	os.precision(2);
#ifdef __APPLE__
	if (_filesize >= 1000000000) { os << ((float)_filesize / 1.0e9f) << " GB"; }
	else if (_filesize >= 1000000) { os << ((float)_filesize / 1.0e6f) << " MB"; }
	else if (_filesize >= 1000) { os << ((float)_filesize / 1.0e3f) << " KB"; }
	else { os << _filesize << " B"; }
#else
	if (_filesize > 1000000000) { os << ((float)_filesize / 1073741824.0f) << " GB"; }
	else if (_filesize > 1000000) { os << ((float)_filesize / 1048576.0f) << " MB"; }
	else if (_filesize > 1000) { os << ((float)_filesize / 1024.0f) << " KB"; }
	else { os << _filesize << " B"; }
#endif
	os << ")";
}

size_t filesize(const char *f) {
#ifdef __CYGWIN__
#define stat64 stat
#elif defined(_WIN32)
#define stat64 _stat32i64
#endif
	struct stat64 s;
	int r = stat64(f, &s);
	return r ? 0 : (size_t)s.st_size;
}

std::string read_status_message(Read_Status status, const char *filename) {
	std::string msg;
	switch (status) {
	case SUCCESS:
		return msg + "Parsed " + filename + "!";
	case FAILURE:
	default:
		return msg + "Could not parse " + filename + "!";
	case CANCELED:
		return msg + "Canceled parsing " + filename + "!";
	case END_OF_FILE:
		return msg + "Could not parse " + filename + "!\nReached end of file unexpectedly.";
	case SIGN_OVERFLOW:
		return msg + "Could not parse " + filename + "!\nA signed integer overflowed.";
	case NO_MEMORY:
		return msg + "Could not parse " + filename + "!\nNot enough memory was available.";
	case NO_TYPES:
		return msg + "Could not parse " + filename + "!\nNo types were defined.";
	case NO_SOMAS:
		return msg + "Could not parse " + filename + "!\nNo somas were defined.";
	case NO_CYCLES:
		return msg + "Could not parse " + filename + "!\nNo cycles were defined.";
	case WRONG_NUM_SOMAS:
		return msg + "Could not parse " + filename + "!\nIncorrect number of somas.";
	case WRONG_NUM_SYNAPSES:
		return msg + "Could not parse " + filename + "!\nIncorrect number of synapses.";
	case WRONG_NUM_CYCLES:
		return msg + "Could not parse " + filename + "!\nIncorrect number of cycles.";
	case BAD_TYPE_LETTER:
		return msg + "Could not parse " + filename + "!\nInvalid soma type letter.";
	case BAD_SOMA_ID:
		return msg + "Could not parse " + filename + "!\nInvalid soma ID.";
	case BAD_SYNAPSE_SOMA_ID:
		return msg + "Could not parse " + filename + "!\nInvalid soma ID for synapse.";
	case BAD_GAP_JUNCTION_SOMA_ID:
		return msg + "Could not parse " + filename + "!\nInvalid soma ID for gap junction.";
	case BAD_SIGNATURE:
		return msg + "Could not parse " + filename + "!\nInvalid file signature.";
	case BAD_VERSION:
		return msg + "Could not parse " + filename + "!\nUnsupported file format version.";
	}
}
