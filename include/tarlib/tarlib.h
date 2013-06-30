#ifndef _TARLIB_TARLIB_H_
#define _TARLIB_TARLIB_H_

#include <stdint.h>

#define TARLIB_VERSION "0.1.0"
#define TARLIB_VERNUM 0x0100
#define TARLIB_VER_MAJOR 0
#define TARLIB_VER_MINOR 1
#define TARLIB_VER_REVISION 0
#define TARLIB_VER_SUBREVISION 0

#ifndef TAREXPORT
#  define TAREXPORT
#endif

#ifndef TAREXTERN
#  define TAREXTERN extern
#endif

#define TAR_HEADER_SIZE 512

// This library does not handle FAR pointers by default. If needed have to convert.

typedef uint8_t Byte;
typedef unsigned int uInt; // See zlib configuration
typedef unsigned long uLong; // See zlib configuration

#define TAR_SYNC_FLUSH    2
#define TAR_BLOCK         5
// Allowed flush values

#define TAR_OK              0
#define TAR_STREAM_END      1
#define TAR_ENTRY_END       2
#define TAR_ERRNO         (-1)
#define TAR_STREAM_ERROR  (-2)
#define TAR_DATA_ERROR    (-3)
#define TAR_MEM_ERROR     (-4)
#define TAR_BUF_ERROR     (-5)
#define TAR_VERSION_ERROR (-6)

enum tar_file {
	TAR_NORMAL = '0',
	TAR_HARD_LINK   = '1',
	TAR_SYM_LINK    = '2',
	// new values only for type flag
	TAR_CHAR_SPEC   = '3',
	TAR_BLOCK_SPEC  = '4',
	TAR_DIR         = '5',
	TAR_FIFO        = '6',
	TAR_CONT_FILE   = '7',
	TAR_G_EX_HEADER = 'g', // global extended header with meta data (POSIX.1-2001)
	TAR_EX_HEADER   = 'x'  // extended header with meta data for the next file in the archive (POSIX.1-2001)
};

typedef struct tar_stream_s {
	const Byte* next_in;  // next input byte(s)
	uInt        avail_in; // number of input bytes available at next_in
	uLong       total_in; // total number of bytes read so far

	const Byte* ptr_out;  // output byte(s)
	uInt        len_out;  // number of output bytes available at ptr_out
	uLong       total_out; // total number of bytes written so far

#if 0
    z_const char *msg;  // last error message, NULL if no error
#endif
	void*         state; // internal state
} tar_stream;

typedef tar_stream* tar_streamp;

// Header is ASCII encoded!
typedef struct tar_header_s {
	char file_name[100];
	char mode[8];
	struct {
		char user[8];
		char group[8];
	} owner_ids;

	char file_bytes_octal[11]; //octal
	char file_bytes_terminator; // null
	char modification_time_octal[12]; //octal

	char header_checksum[8];

	union {
		struct {
			char link_indicator; //file type
			char linked_file_name[100];
		} legacy;
		struct {
			char type_flag;
			char linked_file_name[100];
			char indicator[6]; //"ustar"
			char version[2];  // "00"
			struct {
				char user[32];
				char group[32];
			} owner_names;
			struct {
				char major[8];
				char minor[8];
			} device;
			char filename_prefix[155];
		} ustar;
	} extension;
	char padding[TAR_HEADER_SIZE - 500];
	int8_t done;
	uint64_t file_bytes;
	uint64_t modification_time;
} tar_header;

typedef tar_header* tar_headerp;

#include <tarlib/inflate.h>

#endif // _TARLIB_TARLIB_H_
