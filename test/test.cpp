/*
 * test.cpp
 *
 *  Created on: Feb 4, 2013
 *      Author: Andreas Bergmeier
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <tarlib/tarlib.h>

namespace {

	#define N_ARRAY(x) sizeof(x) / sizeof(*x)

	void one_chunk( tar_stream& stream, tar_header& header, int& file_handle, const Byte* data, size_t size ) {
		stream.next_in  = data;
		stream.avail_in = size;

		int result = TAR_OK;

		while( stream.avail_in != 0 ) {
			// Something to do
			do {
				if( file_handle == -1 && header.done && !tar_headerIsEmpty(&header) ) {

					if( tar_headerIsDir( &header ) == TAR_TRUE ) {
						if( mkdir( header.file_name, S_IRWXU) == -1 ) {
							printf("e%s%d\n", header.file_name, errno);
							assert( errno == EEXIST );
						}
					} else {
						printf( "Creating %s\n", header.file_name );

						file_handle = creat( header.file_name, S_IRUSR | S_IWUSR );
						assert( file_handle != -1 );
					}
				}

				result = tar_inflate( &stream, TAR_HEADER_FLUSH );
				assert( result >= TAR_OK );

				if( file_handle != -1 && stream.len_out ) {
					write( file_handle, stream.ptr_out, stream.len_out );
				}

			} while( result == TAR_OK && stream.avail_in != 0 );

			if( file_handle != -1 && result == TAR_ENTRY_END ) {
				printf( "Closing %s\n", header.file_name );
				assert( close( file_handle ) == 0 );
				file_handle = -1;
			}
		}
	}
} // namespace

int main() {
	tar_stream stream;
	assert( tar_inflateInit(&stream) == TAR_OK );
	assert( stream.len_out == 0 );

	tar_header header;
	tar_inflateGetHeader( &stream, &header );

	int file_handle = -1;

	int tar_handle = open( "test.tar", O_RDONLY );

	std::array<Byte, TAR_HEADER_SIZE> buffer;

	ssize_t read_bytes;
	do {
		read_bytes = read( tar_handle, buffer.data(), buffer.size() );
		one_chunk( stream, header, file_handle, buffer.data(), read_bytes );
	} while( read_bytes > 0 );

	assert( read_bytes == 0 );

	close( tar_handle );

	assert(tar_inflateEnd(&stream) == TAR_OK );
	
	return 0;
}

