#include "tail_wrap.h"

#ifndef __TAIL_FILE_SCAN_H__
#define __TAIL_FILE_SCAN_H__

#define FILE_NAME "test.txt"
#define TAIL_MAX_BUFFER 10

typedef enum scan_enum {
    SCAN_ERROR = -1,
    SCAN_OK,
    SCAN_OK_NOT_ENOUGH,
    SCAN_OK_NO_DATA
} scan_enum_e;

typedef struct data_buffer {
    uint8_t* buffer;
    size_t length;
} data_buffer_t;

void free_data_buffer(data_buffer_t* data_buf);
uint32_t check_for_newline(char* buf, uint32_t len, uint16_t* nlc);
scan_enum_e tail_scan_n_lines(tail_ctx_t* t_ctx, uint16_t nlines,
				data_buffer_t* data_buf);

#endif
