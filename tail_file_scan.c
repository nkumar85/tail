#include "tail_file_scan.h"
#include "tail_log.h"

void free_data_buffer (data_buffer_t* data_buf)
{
	if (data_buf && data_buf->buffer) {
		free(data_buf->buffer);
	}
}

uint32_t check_for_newline (char* buf, uint32_t len, uint16_t* nlc)
{
    while (--len)
    {
	if (buf[len] == '\n') {
	    (*nlc)--;
//		printf("len1 = %u \n", len);
	    return len;
	}
    }

    return 0;
}

scan_enum_e tail_scan_n_lines (tail_ctx_t* t_ctx, uint16_t nlines,
			data_buffer_t* data_buf)
{
    char buf[TAIL_MAX_BUFFER] = {0};
    uint16_t new_line_count = nlines + 1;
    uint32_t len = 0;
    size_t bytes_read = 0, bytes_to_be_read = TAIL_MAX_BUFFER;
	off_t new = 0, temp = 0;

	new = tail_seek_file_offset(t_ctx, 0, SEEK_END);
	temp = new;

    do {
		if (new <= TAIL_MAX_BUFFER) {
	    	bytes_to_be_read = new;
	    	new = 0;
		} else {
		    new -= TAIL_MAX_BUFFER;
		}

        bytes_read = tail_file_read(t_ctx, buf, bytes_to_be_read, new);

		if (bytes_read <= 0) {
		    break;
		}
	
		tail_log(TAIL_LOG_DEBUG, "len1 = %u \n", len);
		len = check_for_newline (buf, (uint32_t)bytes_read, &new_line_count);
        	
		while (new_line_count && len) {
			len = check_for_newline (buf, len, &new_line_count);
		//	printf("len = %u \n", len);
		}
    } while (bytes_read == TAIL_MAX_BUFFER && new_line_count);

    /*
     * How much to copy?
     * OK. Why system call again? Two reasons
     * a) Avoid reallocating buffer again and again.
     * b) Since we have read from disk as much we want,
     *    the data that needs to be copied will be already
     *    in page cache and pread will not take much time IMHO
	 *
	 *    How about large data? Not sure!
     */

//	printf("(temp-(new+len)) = %lu\n", temp - (new+len));
	data_buf->buffer = (uint8_t*)(calloc (sizeof(uint8_t), temp - (new+len)+1));
	tail_file_read(t_ctx, data_buf->buffer, temp - (new+len), new+len);
	data_buf->buffer[temp - (new+len)] = '\0';
	data_buf->length = temp - (new+len);
    
//	printf("Current Offset = %lu, New Line Count = %u\n", (new+len),
//			new_line_count);

    if (new_line_count) {
	/* OK But not enough lines were found */
		return SCAN_OK_NOT_ENOUGH;
    }

	return SCAN_OK;
}

#if 0
int main()
{
    tail_file_handle_t f_handle;
    memset(&f_handle, 0, sizeof(tail_file_handle_t));

    f_handle.fd = open(FILE_NAME, O_RDONLY, 0x644);    
    fstat(f_handle.fd, &f_handle.f_stat);

    //Check for zero length file

    data_buffer_t data_buf;
    tail_scan_n_lines(&f_handle, 5, &data_buf);

    printf("%s", data_buf.buffer);
}
#endif
