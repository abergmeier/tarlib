#ifndef _TARLIB_INFLATE_H_
#define _TARLIB_INFLATE_H_

/*
 * Initializes tar_stream.
 * Resets all state.
 */
TAREXTERN int TAREXPORT tar_inflateInit(tar_streamp strm);

/*
 * Tar format is a continuous list of header and content blocks.
 * Important for users coming from zlib:
 *   Contrary to zlib, tarlib does not copy output content to a buffer.  next_out  is just a pointer to
 *     next_in  .
 * 1. This function builds the header first. When bytes belong to header  next_in  is advanced (read in) and
 *     avail_in  decreased by the count.  avail_out  is not set since the header is only available
 *    when it was read in its entirety. Once it is available  header  is non null.
 *
 * 2. After all bytes of file entry header were processed, next are the content bytes.  avail_out  
 *    is set to  avail_in  but not to more than the file size and  next_out  is set to  next_in  .
 *    Once size of the current file entry is reached the function returns  TAR_ENTRY_END  .
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

/*
 * Cleans up tar_stream
 */
TAREXTERN int TAREXPORT tar_inflateEnd (tar_streamp strm);



#endif //_TARLIB_INFLATE_H_

