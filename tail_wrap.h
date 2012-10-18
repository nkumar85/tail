#ifndef __TAIL_WRAP_H__
#define __TAIL_WRAP_H__

#define __USE_FILE_OFFSET64

#include <signal.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "tail_log.h"

typedef void (*sig_handler)(int signum);
typedef _Bool BOOL;

#define B_FALSE 0
#define B_TRUE 1

#define TAIL_MEM_ZALLOC(type, count, size)\
		(type*)(calloc(count, size))

#define TAIL_MEM_FREE(ptr)\
		if (ptr) {		  \
			free(ptr);	  \
			ptr = NULL;	  \
		}

/* Just for OS abstraction */
#define TAIL_IN_ACCESS       	IN_ACCESS       
#define TAIL_IN_ATTRIB       	IN_ATTRIB                
#define TAIL_IN_CLOSE_WRITE		IN_CLOSE_WRITE
#define TAIL_IN_CLOSE_NOWRITE	IN_CLOSE_NOWRITE
#define TAIL_IN_CREATE			IN_CREATE       
#define TAIL_IN_DELETE			IN_DELETE  
#define TAIL_IN_DELETE_SELF		IN_DELETE_SELF
#define TAIL_IN_MODIFY			IN_MODIFY    
#define TAIL_IN_MOVE_SELF		IN_MOVE_SELF   
#define TAIL_IN_MOVED_FROM		IN_MOVED_FROM
#define TAIL_IN_MOVED_TO		IN_MOVED_TO
#define TAIL_IN_OPEN			IN_OPEN

#define TAIL_CUR_FUNC __func__
#define TAIL_EVENT_MASK (IN_MODIFY|IN_DELETE_SELF|IN_MOVE_SELF|IN_UNMOUNT)
#define TAIL_FILE_HANDLE(ctx) (&(ctx->f_handle))

typedef uint32_t tail_evt_mask;

typedef enum tail_return {
	TAIL_OK,			/* Everything is OK */		
	TAIL_INVALID_CTX,	/* Invalid context passed */
	TAIL_NOT_REG_FILE,	/* Not a regular file */
	TAIL_MALLOC_ERROR,	/* Memory allocation failed */
	TAIL_SYS_ERR,		/* System call error */
} tail_return_e;

typedef struct tail_file_handle {
	struct stat f_stat;
	int fd;
} tail_file_handle_t;

typedef struct file_evt_handle {
	int evt_fd;
	int watch_fd;
	struct inotify_event i_evt;
} file_evt_handle_t;

typedef struct sig_vector {
	sig_handler sig_f_handle;
} sig_vector_t;

typedef struct tail_ctx {
	tail_file_handle_t f_handle;
	file_evt_handle_t f_evt_handle;
	struct {
		off_t cur_off;
	} priv_data;
} tail_ctx_t;

tail_return_e tail_map_sys_err (const char* func, const char* syscall_text);
BOOL tail_setup_signals(sig_vector_t* sig_vec);

tail_return_e tail_file_open (tail_ctx_t* t_ctx, const char* f_name);
off_t tail_seek_file_offset (tail_ctx_t* t_ctx, off_t offset, int pos);
ssize_t tail_file_read (tail_ctx_t* t_ctx, void* buf, size_t n_bytes, off_t offset);
BOOL tail_is_regular_file (tail_ctx_t* t_ctx);
BOOL tail_refresh_file (tail_ctx_t* t_ctx);
off_t tail_file_length (tail_ctx_t* t_ctx);
tail_return_e tail_read_diff (tail_ctx_t* t_ctx, void* buf, ssize_t* len);
void tail_file_close (tail_ctx_t* t_ctx);

tail_return_e tail_file_event_init (tail_ctx_t* t_ctx);
tail_return_e tail_create_watch (tail_ctx_t* t_ctx, const char* f_name, uint32_t mask);
tail_return_e tail_event_watch (tail_ctx_t* t_ctx);
tail_evt_mask tail_get_event_mask (tail_ctx_t* t_ctx);
void tail_file_event_deinit (tail_ctx_t* t_ctx);

tail_ctx_t* tail_ctx_init (void);
void tail_ctx_deinit (tail_ctx_t* t_ctx);

#endif
