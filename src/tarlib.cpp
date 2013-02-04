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

	template <unsigned int OFFSET, unsigned int COUNT>
	unsigned long long _base256_to_10( typename std::enable_if<COUNT == 0, const Byte*>::type ptr) {
		return ptr[OFFSET];
	}

	template <unsigned int OFFSET, unsigned int COUNT>
	unsigned long long _base256_to_10( typename std::enable_if<COUNT != 0, const Byte*>::type ptr) {
		return (ptr[OFFSET] * 256 * COUNT) + _base256_to_10<OFFSET + 1, COUNT - 1>( ptr );
	}

	template <unsigned int OFFSET, unsigned int COUNT>
	unsigned long long base256_to_10( const Byte* ptr) {
		//return _base256_to_10<OFFSET, COUNT - 1>( ptr );
		return std::strtoull( reinterpret_cast<const char*>(ptr), nullptr, 256 );
	}

	unsigned long base8_to_10( const Byte* ptr ) {
		return std::strtoul( reinterpret_cast<const char*>(ptr), nullptr, 8 );
	}

	// Converts octal to usable decimal values
	void convert( const tar_header& header, tar_stream& strm ) {
		const bool is_base_256 = *header.file_bytes_octal & 0xFF;
		
		std::uint64_t value;
		
		if( is_base_256 ) {
			const auto& field = header.file_bytes_octal;
			value = base256_to_10<1, 10>( field );
		} else {
			std::ostringstream stream;
			value = base8_to_10( std::begin(header.file_bytes_octal) );
		}
		strm.file_bytes = value;
	
		strm.modification_time = base8_to_10( header.modification_time_octal );
	}
	
	// Internal tar state
	struct internal {
		internal();
		bool put( tar_stream& strm );
	private:
		tar_header    _header_buffer;
		Byte*         _header_ptr;
		std::uint64_t _left;
	};
	
	internal& intern( tar_stream& strm ) {
		return *static_cast<internal*>( strm.internal );
	}

	Byte* begin( tar_header& header ) {
		return reinterpret_cast<Byte*>( &header );
	}

	Byte* end( tar_header& header ) {
		return begin(header) + sizeof(header);
	}
}

internal::internal() :
	_header_buffer(), _header_ptr( reinterpret_cast<Byte*>(&_header_buffer) ), _left( 0 )
{
}

bool
internal::put( tar_stream& strm ) {
	const auto buffer_end = end(_header_buffer );
	if( _header_ptr == buffer_end && !_left ) {
		if( strm.avail_in ) {
			// Go to the next entry
			_header_ptr = begin( _header_buffer );
			strm.next_out  = nullptr;
			strm.avail_out = 0;
		} else
			return false; //end of entry
	}

	if( _header_ptr != buffer_end ) {
		// Try to make the header full
		const auto header_distance = std::distance( _header_ptr, buffer_end );
		assert( header_distance >= 0 );
		const auto distance = std::min( static_cast<decltype(strm.avail_in)>(header_distance), strm.avail_in );
		
		std::copy( strm.next_in, strm.next_in + distance, _header_ptr );
		strm.next_in  += distance;
		strm.avail_in -= distance;
		strm.total_in += distance;
		_header_ptr   += distance;
		
		if( _header_ptr == buffer_end ) {
			// We reached a full header
			const tar_header& header = *reinterpret_cast<tar_header*>( begin(_header_buffer) );
			convert( header, strm );
			strm.header = &header;
			_left = strm.file_bytes;
		}
	}

	if( _header_ptr == buffer_end ) {
		// Process file entry content
		
		if( strm.avail_in ) {
			// There is actual data here

			// Limit to current file entry only
			strm.avail_out = std::min( strm.avail_in, static_cast<decltype(strm.avail_in)>(_left) );
			strm.next_out  = strm.next_in;
			strm.total_out += strm.avail_out;
			
			strm.avail_in -= strm.avail_out;
			strm.next_in  += strm.avail_out;
			strm.total_in += strm.avail_out;

			_left         -= strm.avail_out;
		}
		
		if( !_left )
			return false; // end of entry
	}
	
	// Continue processing current entry
	return true;
}

// C wrapper
int TAREXPORT
tar_inflateInit( tar_streamp strm ) {
	assert( strm );
	strm->next_in           = nullptr;
	strm->avail_in          = 0;
	strm->next_out          = nullptr;
	strm->avail_out         = 0;
	strm->internal          = new internal();
	strm->header            = nullptr;
	strm->file_bytes        = 0;
	strm->modification_time = 0;
	return TAR_OK;
}

int TAREXPORT
tar_inflate( tar_streamp strm ) {
	assert( strm );
	if( intern(*strm).put( *strm ) )
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
	strm->internal = nullptr;
	return TAR_OK;
}



