#ifndef _TARLIB_TARLIB_H_
#define _TARLIB_TARLIB_H_

#include <stdint.h>

#define TARLIB_VERSION "0.1.0"
#define TARLIB_VERNUM 0x0100
#define TARLIB_VER_MAJOR 0
#define TARLIB_VER_MINOR 1
#define TARLIB_VER_REVISION 0
#define TARLIB_VER_SUBREVISION 0

#define TAR_HEADER_SIZE 512

// This library does not handle FAR pointers by default. If needed have to convert.

typedef uint8_t Byte;
typedef unsigned int uInt; // See zlib configuration
typedef unsigned long uLong; // See zlib configuration

#define TAR_OK              0
#define TAR_STREAM_END      1
#define TAR_ENTRY_END       2
#define TAR_ERRNO         (-1)
#define TAR_STREAM_ERROR  (-2)
#define TAR_DATA_ERROR    (-3)
#define TAR_MEM_ERROR     (-4)
#define TAR_BUF_ERROR     (-5)
#define TAR_VERSION_ERROR (-6)

struct tar_header {
	Byte file_name[100]
	uint64_t mode;
	struct {
		uint64_t user;
		uint64_t group;
	} owner_ids;

	Byte file_bytes_octal[12]; //octal
	Byte modification_time_octal[12]; //octal

	uint64_t header_checksum;
	
	union {
		struct {
			Byte link_indicator; //file type
			Byte linked_file_name[100];
			Byte padding[TAR_HEADER_SIZE - 257];
		} legacy;
		struct {
			Byte type_flag;
			Byte linked_file_name[100];
			Byte indicator[6]; //"ustar"
			Byte version[2]; // "00"
			struct {
				Byte user[32];
				Byte group[32];
			} owner_names;
			struct {
				uint64_t major;
				uint64_t minor;
			} device;
			Byte filename_prefix[155];
			Byte padding[TAR_HEADER_SIZE - 500];
		} ustar;
	} extension;
};

typedef struct tar_stream_s {
	z_const Bytef *next; // next byte
	uInt     avail_in;   // number of input bytes available at next
	uLong    total;      // total number of bytes read so far
	uInt     avail_out;  // number of output bytes available at next
	
#if 0
    z_const char *msg;  // last error message, NULL if no error
#endif
	const tar_header* header;
	uint64_t file_bytes;
	uint64_t modification_time;
	void* internal; // internal state
} tar_stream;

typedef tar_stream* tar_streamp;

/*
 * Initializes tar_stream.
 * Resets all state.
 */
TAREXTERN int TAREXPORT tar_inflateInit(tar_streamp strm);

/*
 * Tar format is a continuous list of header and content blocks.
 * 1. This function builds the header first. When bytes belong to header  next  is advanced (read in) and
 *    avail_in  decreased by the count.  avail_out  is not set since the header is only available
 *    when it was read in its entirety. Once it is available  header  is non null.
 *
 * 2. After all bytes of file entry header were processed, next are the content bytes.  avail_out  
 *    is set to  avail_in  but not to more than the file size. Once size of the current file entry 
 *    is reached the function returns  TAR_ENTRY_END  .
 *
 * 3. After  TAR_ENTRY_END was returned invoking this function with data advances to the next file
 *    entry. Then  header  is null and fields are reset.
 * 
 * Since fields: _file_size_ (in bytes) and _modification_time_ (in unix format) are in octal format
 * they get converted for every header. See tar_stream_s fields: file_types and modification_time.
 *
 * You should at least provide  avail_in  >= 512 bytes, since then the file entry header can be copied
 * in one go.
 */
TAREXTERN int TAREXPORT tar_inflate    (tar_streamp strm);
TAREXTERN int TAREXPORT tar_inflateEnd (tar_streamp strm);



#endif //_TARLIB_TARLIB_H_

