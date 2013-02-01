tarlib
======

A passive, non blocking, in memory tar inflate library. Inspired by zlib.
C and C++ bindings available.

Example:
    tar_stream stream;
    tar_inflateInit( &stream );
    .
    .
    .
    stream.next_in  = data;
    stream.avail_in = data_size;
    
    int file_handle;
    
    // Check avail_in because  tar_inflate  might not process all.
    // It just processes all data for the current file entry.
    while( stream.avail_in ) {
    	int result = tar_inflate( &stream );
    	
    	if( result < TAR_OK ) {
    		printf( "Error");
    	}
    	
    	if( stream.header ) {
    		printf( "Creating %s", stream.header.file_name );
    		file_handle = open( stream.header.file_name, O_WRONLY );
    	}
    
    	
    	if( stream.avail_out ) {
    		printf( "Writing %d", stream.avail_out );
    		write( file_handle, _stream.next_out, stream.avail_out );
    	}
    	
    	if( result == TAR_ENTRY_END ) {
    		printf( "Closing %s", stream.header.file_name );
    		close( file_handle );
    	}
    }
    
