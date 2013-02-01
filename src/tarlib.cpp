#include <string>
#include <array>

using namespace tarlib;

inflater::inflater( header_callback_type header_callback, progress_callback_type progress ) :
	_header_callback( header_callback ), _progress_callback( progress )
{
}

void
inflater::put( const std::uint8_t* begin, const std::uint8_t* end ) {
	_tarstream.next     = begin;
	_tarstream.avail_in = std::distance( begin, end );
	
	while( _tarstream.avail_in ) {
		int result = tar_inflate( &_tarstream );
	
		if( !_invoked && _tarstream.header ) {
			_invoked = true;
			_header_callback( *_tarstream.header );
		}
	
		if( _tarstream.avail_out ) {
			_process( _tarstream.next, _tarstream.next + _tarstream.avail_out );
		}
	}
	
	// All data was processed
}

namespace {
	// Converts octal to usable decimal values
	void convert( const tar_header& header, tar_stream& strm ) {
		std::stringstream stream( std::begin(header.file_size.octal_bytes), std::end(header.file_size.octal_bytes) );
		stream >> std::oct >> strm.file_bytes;
	
		stream.str( header.modifiction_time.octal_unix );
		stream >> std::oct >> strm.modification_time;
	}
	
	// Internal tar state
	struct internal {
		internal();
		put( tar_stream& strm );
	private:
		tar_header    _header_buffer;
		Byte*         _header_ptr;
		std::uint64_t _left;
	};
	
	internal& inflater( tar_stream& strm ) {
		return *static_cast<internal*>( strm.internal );
	}
}

internal::internal() :
	_header_buffer(), _header_ptr( &_header_buffer ), _left( 0 )
{
}

bool
internal::put( tar_stream& strm ) {
	const auto buffer_end = std::end(_header_buffer);
	if( _header_ptr == buffer_end && !_left ) {
		if( strm.avail_in ) {
			// Go to the next entry
			_header_ptr = std::begin( _header_buffer );
		} else
			return false; //end of entry
	}

	if( _header_ptr != buffer_end ) {
		// Try to make the header full
		auto distance = std::distance( _header_ptr, buffer_end );
		distance = std::min( distance, strm.avail_in );
		
		std::copy( strm.next, strm.next + distance, _header_ptr );
		strm.next     += distance;
		strm.avail_in -= distance;
		strm.total    += distance;
		_header_ptr   += distance;
		
		if( _header_ptr == buffer_end ) {
			// We reached a full header
			const tar_header& header = *reinterpret_cast<tar_header*>( std::begin(_header_buffer) );
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
			strm.avail_out = std::min( strm.avail_in, _left);
			strm.avail_in -= strm.avail_out;
			strm.total    += strm.avail_out;
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
tar_extractInit( tar_streamp strm ) {
	assert( strm )
	strm->next              = nullptr;
	strm->avail_in          = 0;
	strm->avail_out         = 0;
	strm->internal = new tarlib::inflater();
	strm->header            = nullptr;
	strm->file_bytes        = 0;
	strm->modification_time = 0;
}

int TAREXPORT
tar_extract( tar_streamp strm ) {
	assert( strm )
	if( inflater(strm).put( strm ) )
	else
		return Z_ENTRY_END;
}

int TAREXPORT
tar_extractEnd( tar_streamp strm ) {
	assert( strm )
	delete strm->internal;
	// Signal to externals that the stream
	// has ended
	strm->internal = nullptr;
}



