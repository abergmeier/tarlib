#include <string>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <type_traits>
#include <sstream>
#include <iterator>
#include <tarlib/tarlib.h>

namespace {
#if 0
	template <unsigned int OFFSET, unsigned int COUNT>
	unsigned long long _base256_to_10( typename std::enable_if<COUNT == 0, const Byte*>::type ptr) {
		return ptr[OFFSET];
	}

	template <unsigned int OFFSET, unsigned int COUNT>
	unsigned long long _base256_to_10( typename std::enable_if<COUNT != 0, const Byte*>::type ptr) {
		return (ptr[OFFSET] * 256 * COUNT) + _base256_to_10<OFFSET + 1, COUNT - 1>( ptr );
	}
#endif
	template <unsigned int OFFSET, unsigned int COUNT>
	unsigned long long base256_to_10( const char* ptr) {
		//return _base256_to_10<OFFSET, COUNT - 1>( ptr );
		return std::strtoull( ptr, nullptr, 256 );
	}

	unsigned long base8_to_10( const char* ptr ) {
		return std::strtoul( ptr, nullptr, 8 );
	}

	// Converts octal to usable decimal values
	void convert( tar_header& header ) {

		// Make sure file name is properly \0 terminated
		header.file_name[TAR_NELEMENTS(header.file_name) - 1] = '\0';

		const bool is_base_256 = header.file_bytes_octal[0] & 0x80;
		
		std::uint64_t value;
		
		const auto& field = header.file_bytes_octal;
		// Enforce proper file_bytes encoding
		header.file_bytes_terminator = '\0';
		if( is_base_256 ) {
			value = base256_to_10<1, 10>( field );
		} else {
			//std::ostringstream stream;
			value = base8_to_10( field );
		}
		header.file_bytes = value;
		header.modification_time = base8_to_10( header.modification_time_octal );
	}
	
	// Internal tar state
	struct internal {
		internal();
		tar_headerp header( tar_headerp );
		bool put( tar_stream& strm, bool continueAfterHeader );
	private:
		void reset( tar_stream& strm );
		struct {
			tar_headerp ptr_begin;
			Byte*       ptr;
			uint16_t    left; //bytes left till header is full
			void reset() {
				ptr  = reinterpret_cast<Byte*>( ptr_begin );
				left = TAR_HEADER_SIZE;
				if( ptr_begin ) {
					ptr_begin->done = 0;
					ptr_begin->file_bytes        = 0;
					ptr_begin->modification_time = 0;
				}
			}

			void reset( tar_headerp header ) {
				ptr_begin = header;
				reset();
			}
		} _header;
		tar_header    _internal_header;
		std::uint64_t _left;
		std::uint16_t _endPadding;
	};
	
	internal& intern( tar_stream& strm ) {
		return *static_cast<internal*>( strm.state );
	}

	Byte* begin( tar_header& header ) {
		return reinterpret_cast<Byte*>( &header );
	}

	Byte* end( tar_header& header ) {
		return begin(header) + TAR_HEADER_SIZE;
	}
}

internal::internal() :
	_header(), _internal_header(), _left( 0 )
{
	_header.reset( &_internal_header );
}

tar_headerp
internal::header( tar_headerp header ) {
	_header.reset( header );
	return _header.ptr_begin;
}

void
internal::reset( tar_stream& strm ) {
	_header.reset();
	strm.ptr_out  = nullptr;
	strm.len_out = 0;
	_left = 0;
}

bool
internal::put( tar_stream& strm, bool continueAfterHeader ) {
	if( !_header.left && !_left ) {
		if( strm.avail_in ) {
			// Go to the next entry
			reset( strm );
		}

		return false; //end of entry
	}

	if( _header.left ) {
		// Try to make the header full
		const auto distance = std::min( static_cast<decltype(strm.avail_in)>(_header.left), strm.avail_in );
		assert( distance <= _header.left );
		
		if( _header.ptr ) {
			std::copy( strm.next_in, strm.next_in + distance, _header.ptr );
			_header.ptr += distance;
		}
		strm.next_in  += distance;
		strm.avail_in -= distance;
		strm.total_in += distance;
		// We made sure distance is at max _header.left
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
		_header.left  -= distance;
#pragma GCC diagnostic pop
		
		if( !_header.left ) {
			// We reached a full header

			if( _header.ptr_begin ) {
				tar_header& header = *_header.ptr_begin;
				convert( header );
				_header.ptr_begin->done = 1;
			}
			//FIXME: Make working without internal header
			_left       = _header.ptr_begin->file_bytes;
			_endPadding = [&]() {
				// Data is always rounded up to 512
				static const std::uint16_t PADDING_SIZE = 512;
				auto quot = _left / PADDING_SIZE;
				if( _left % PADDING_SIZE ) {
					++quot;
				}
				return static_cast<uint16_t>( quot * PADDING_SIZE - _left );
			}();
			_left += _endPadding;

			if( !continueAfterHeader )
				return true;
		}
	}

	if( !_header.left && strm.avail_in ) {
		// Process file entry content
		strm.ptr_out    = strm.next_in;
		
		uInt moveValue = [&]() {
			if( _left > _endPadding ) {
				const std::uint64_t leftContent = _left - _endPadding;
				// Limit to current file entry only

				strm.len_out    = strm.avail_in < leftContent ? strm.avail_in : static_cast<decltype(strm.avail_in)>( leftContent );
				assert( strm.len_out <= leftContent );
				strm.total_out += strm.len_out;

				return strm.len_out;
			} else {
				const uInt leftPadding = static_cast<decltype(_endPadding)>( _left );
				// Consume the padding but do not generate output
				strm.len_out    = 0;
				strm.total_out += leftPadding;

				return leftPadding;
			}
		}();

		strm.avail_in -= moveValue;
		strm.next_in  += moveValue;
		strm.total_in += moveValue;

		_left         -= moveValue;
	}

	return true;
}

// C wrapper
int TAREXPORT
tar_inflateInit( tar_streamp strm ) {
	assert( strm );

	strm->total_in  = 0;
	strm->ptr_out   = nullptr;
	strm->len_out   = 0;
	strm->total_out = 0;
	strm->state     = new internal();
	return TAR_OK;
}

int TAREXPORT
tar_inflate( tar_streamp strm, int flush ) {
	assert( strm );

	if( intern(*strm).put( *strm, flush != TAR_HEADER_FLUSH ) )
		return TAR_OK;
	else
		return TAR_ENTRY_END;
}

int TAREXPORT
tar_inflateEnd( tar_streamp strm ) {
	assert( strm );
	delete &intern( *strm );
	// Signal to externals that the stream
	// has ended
	strm->state = nullptr;
	return TAR_OK;
}

int TAREXPORT
tar_inflateGetHeader( tar_streamp strm,
                      tar_headerp head ) {
	assert(strm);
	assert(head);
	auto& internal = intern( *strm );
	internal.header( head );
	return TAR_OK;
}

int TAREXPORT
tar_inflateReset ( tar_streamp strm ) {
	assert(strm);
	auto& internal = intern( *strm );
	internal.header( nullptr );
	return TAR_OK;
}

int TAREXPORT
tar_headerIsDir( tar_headerp header ) {
	assert( header );

	if( !header->done )
		return -1;

	if( header->extension.ustar.type_flag == TAR_DIR )
		return TAR_TRUE;

	// support pre-POSIX.1-1988
	auto len = strlen( header->file_name );

	if( len == 0 )
		return TAR_FALSE;

	// Check indicating via terminating slash
	return header->file_name[len - 1] == '/' ? TAR_TRUE : TAR_FALSE;
}



